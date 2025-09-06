#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "RBTree.h"

using namespace std;

class KVStore {
private:
    RBTree<string, string> memtable;
    size_t memtableSize = 0;
    const size_t MEMTABLE_THRESHOLD = 1024;
    int sstableCounter = 0;
    string walPath = "temp/wal.log";

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
        ofstream sstableFile(filename);

        if (!sstableFile.is_open()) {
            cerr << "Error: Could not open SSTable file for writing: " << filename << endl;
            return;
        }

        vector<pair<string, string>> sortedData = memtable.getSortedData();

        for (const auto& pair : sortedData) {
            sstableFile << pair.first << " " << pair.second << "\n";
        }

        sstableFile.close();
        memtable.clear();
        memtableSize = 0;

        // Clear the WAL file after successful flush
        ofstream walFile(walPath, ofstream::out | ofstream::trunc);
        walFile.close();

        cout << "[INFO] Memtable flushed to " << filename << " and WAL cleared." << endl;
    }

public:
    KVStore() {
        // Create temp directory if it doesn't exist
        system("mkdir temp 2>nul"); 
        recoverFromWAL();
    }

    void insertKey(const string& key, const string& value) {
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
        cout << "Inserted. Current memtable size: " << memtableSize << " bytes." << endl;

        if (memtableSize > MEMTABLE_THRESHOLD) {
            flushToSSTable();
        }
    }

    string getKey(const string& key) {
        // 1. Search in memtable
        try {
            string value = memtable.search(key);
            return value;
        } catch (const runtime_error& e) {
            // Key not in memtable, proceed to search SSTables
        }

        // 2. Search in SSTables (from newest to oldest)
        for (int i = sstableCounter - 1; i >= 0; --i) {
            string filename = "sstable_" + to_string(i) + ".txt";
            ifstream sstableFile(filename);
            if (!sstableFile.is_open()) {
                continue;
            }

            string line;
            while (getline(sstableFile, line)) {
                stringstream ss(line);
                string fileKey, fileValue;
                ss >> fileKey;
                getline(ss, fileValue);
                // Remove leading space from value
                if (!fileValue.empty() && fileValue[0] == ' ') {
                    fileValue.erase(0, 1);
                }

                if (fileKey == key) {
                    sstableFile.close();
                    return fileValue;
                }
            }
            sstableFile.close();
        }

        return "Key not found.";
    }
};