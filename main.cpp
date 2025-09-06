#include "server.cpp"

int main() {
    KVStore store; // Create an instance of your KVStore

    // Start the web server and pass the KVStore instance to it
    start_web_server(store);

    return 0;
}

