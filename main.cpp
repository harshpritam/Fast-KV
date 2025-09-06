#include "KVStore.cpp"

void printUsage() {
    cout << "Usage:" << endl;
    cout << "  insert <key> <value>" << endl;
    cout << "  get <key>" << endl;
    cout << "  exit" << endl;
}

int main() {
    KVStore store;
    string line;

    cout << "Welcome to Fast-KV!" << endl;
    printUsage();

    while (true) {
        cout << "> ";
        getline(cin, line);

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "insert") {
            string key;
            string value;
            ss >> key;
            getline(ss, value);

            // Remove leading space from value
            if (!value.empty() && value.front() == ' ') {
                value.erase(0, 1);
            }

            if (key.empty() || value.empty()) {
                cout << "Error: 'insert' requires a key and a value." << endl;
            } else {
                store.insertKey(key, value);
            }
        } else if (command == "get") {
            string key;
            ss >> key;
            if (key.empty()) {
                cout << "Error: 'get' requires a key." << endl;
            } else {
                cout << store.getKey(key) << endl;
            }
        } else if (command == "exit") {
            break;
        } else if (!command.empty()) {
            cout << "Unknown command: " << command << endl;
            printUsage();
        }
    }

    return 0;
}
