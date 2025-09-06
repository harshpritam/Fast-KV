#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <chrono> // For timing
#include "RBTree.h"

using namespace std;
using namespace chrono;

// Represents the in-memory index for a single SSTable
struct SSTableIndex {
    string filename;
    map<string, streampos> sparseIndex; // Maps a key to its file offset
};

class KVStore {
private:
    RBTree<string, string> memtable;
    size_t memtableSize = 0;
    const size_t MEMTABLE_THRESHOLD = 1024;
    const size_t INDEX_INTERVAL = 128; // Create an index entry every 128 bytes
    int sstableCounter = 0;
    string walPath = "temp/wal.log";
    const string TOMBSTONE = "---DELETED---";
    vector<SSTableIndex> sstableIndices;

    void recoverFromWAL() {
        ifstream walFile(walPath);
        if (!walFile.is_open()) {
            return; // No WAL file, nothing to recover
        }

        cout << "[INFO] Starting recovery from WAL..." << endl;
        string line;
        while (getline(walFile, line)) {
            stringstream ss(line);
            string key, value;
            ss >> key;
            getline(ss, value);
            if (!value.empty() && value[0] == ' ') {
                value.erase(0, 1);
            }
            
            // Directly insert into memtable without logging again
            memtable.insert(key, value);
            memtableSize += key.length() + value.length();
        }
        walFile.close();
        cout << "[INFO] WAL recovery finished. Memtable size: " << memtableSize << " bytes." << endl;

        if (memtableSize > MEMTABLE_THRESHOLD) {
            flushToSSTable();
        }
    }

    void flushToSSTable() {
        if (memtable.empty()) {
            return;
        }

        string filename = "sstable_" + to_string(sstableCounter++) + ".txt";
        ofstream sstableFile(filename, ios::binary); // Open in binary mode for accurate streampos

        if (!sstableFile.is_open()) {
            cerr << "Error: Could not open SSTable file for writing: " << filename << endl;
            return;
        }

        SSTableIndex newIndex;
        newIndex.filename = filename;
        streampos lastIndexPos = 0;

        vector<pair<string, string>> sortedData = memtable.getSortedData();

        for (const auto& pair : sortedData) {
            streampos currentPos = sstableFile.tellp();
            if (newIndex.sparseIndex.empty() || static_cast<size_t>(currentPos - lastIndexPos) >= INDEX_INTERVAL) {
                newIndex.sparseIndex[pair.first] = currentPos;
                lastIndexPos = currentPos;
            }
            sstableFile << pair.first << " " << pair.second << "\n";
        }

        sstableFile.close();
        sstableIndices.push_back(newIndex); // Add the new index to our in-memory list
        memtable.clear();
        memtableSize = 0;

        // Clear the WAL file after successful flush
        ofstream walFile(walPath, ofstream::out | ofstream::trunc);
        walFile.close();

        cout << "[INFO] Memtable flushed to " << filename << " and WAL cleared. Index created." << endl;
    }

public:
    KVStore() {
        // Create temp directory if it doesn't exist
        system("mkdir temp 2>nul"); 
        recoverFromWAL();
    }

    void insertKey(const string& key, const string& value) {
        auto start = high_resolution_clock::now();

        // 1. Log to WAL first
        ofstream walFile(walPath, ios::app);
        if (!walFile.is_open()) {
            cerr << "Error: Could not open WAL file for writing." << endl;
            return;
        }
        walFile << key << " " << value << "\n";
        walFile.close();

        // 2. Insert into memtable
        memtable.insert(key, value);
        memtableSize += key.length() + value.length();
        
        auto end = high_resolution_clock::now();
        duration<double, milli> duration = end - start;
        cout << "[PERF] insertKey for '" << key << "' took " << duration.count() << " ms. Memtable size: " << memtableSize << " bytes." << endl;

        if (memtableSize > MEMTABLE_THRESHOLD) {
            cout << "[INFO] Memtable threshold reached. Flushing to SSTable..." << endl;
            flushToSSTable();
        }
    }

    void deleteKey(const string& key) {
        // 1. Log tombstone to WAL
        ofstream walFile(walPath, ios::app);
        if (!walFile.is_open()) {
            cerr << "Error: Could not open WAL file for writing." << endl;
            return;
        }
        walFile << key << " " << TOMBSTONE << "\n";
        walFile.close();

        // 2. Insert tombstone into memtable
        memtable.insert(key, TOMBSTONE);
        memtableSize += key.length() + TOMBSTONE.length();
        cout << "Deleted key '" << key << "'. Current memtable size: " << memtableSize << " bytes." << endl;

        if (memtableSize > MEMTABLE_THRESHOLD) {
            flushToSSTable();
        }
    }

    string getKey(const string& key) {
        // 1. Search in memtable
        try {
            string value = memtable.search(key);
            if (value == TOMBSTONE) {
                cout << "[INFO] Key '" << key << "' found in memtable as a tombstone." << endl;
                return "Key not found.";
            }
            cout << "[INFO] Key '" << key << "' found in memtable." << endl;
            return value;
        } catch (const runtime_error& e) {
            // Key not in memtable, proceed to search SSTables
        }

        cout << "[INFO] Key '" << key << "' not in memtable. Searching SSTables..." << endl;
        auto start = high_resolution_clock::now();

        // 2. Search in SSTables using the sparse index (from newest to oldest)
        for (auto it = sstableIndices.rbegin(); it != sstableIndices.rend(); ++it) {
            const auto& index = *it;
            
            // Find the latest key in the sparse index that is less than or equal to the target key
            auto sparseIt = index.sparseIndex.upper_bound(key);
            if (sparseIt != index.sparseIndex.begin()) {
                --sparseIt; // This gives us the starting point for our scan
            } else if (!index.sparseIndex.empty() && key < index.sparseIndex.begin()->first) {
                // If the key is smaller than the first indexed key, it won't be in this file
                continue;
            }


            ifstream sstableFile(index.filename);
            if (!sstableFile.is_open()) {
                cerr << "[ERROR] Could not open SSTable file: " << index.filename << endl;
                continue;
            }

            // Seek to the offset provided by the sparse index
            if (sparseIt != index.sparseIndex.end()) {
                sstableFile.seekg(sparseIt->second);
            }


            string line;
            while (getline(sstableFile, line)) {
                stringstream ss(line);
                string fileKey, fileValue;
                ss >> fileKey;
                
                if (sparseIt != index.sparseIndex.end()) {
                    auto nextIt = next(sparseIt, 1);
                    if (nextIt != index.sparseIndex.end() && fileKey >= nextIt->first) {
                        // Optimization: if we've passed our key and are into the next indexed block, stop.
                        break;
                    }
                }

                getline(ss, fileValue);
                // Remove leading space from value
                if (!fileValue.empty() && fileValue[0] == ' ') {
                    fileValue.erase(0, 1);
                }

                if (fileKey == key) {
                    sstableFile.close();
                    auto end = high_resolution_clock::now();
                    duration<double, milli> duration = end - start;
                    cout << "[PERF] SSTable read for '" << key << "' took " << duration.count() << " ms." << endl;

                    if (fileValue == TOMBSTONE) {
                        return "Key not found.";
                    }
                    return fileValue;
                }
            }
            sstableFile.close();
        }

        auto end = high_resolution_clock::now();
        duration<double, milli> duration = end - start;
        cout << "[PERF] SSTable search for '" << key << "' (not found) took " << duration.count() << " ms." << endl;

        return "Key not found.";
    }
};