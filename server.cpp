#include "httplib.h"
#include "KVStore.cpp"
#include <chrono>
#include <iostream>

// This function sets up and runs the web server.
void start_web_server(KVStore& store) {
    httplib::Server svr;

    // Endpoint for inserting a key-value pair
    svr.Post("/insert", [&](const httplib::Request& req, httplib::Response& res) {
        auto start = chrono::high_resolution_clock::now();
        cout << "[REQUEST] " << req.method << " " << req.path << endl;

        if (req.has_param("key") && req.has_param("value")) {
            string key = req.get_param_value("key");
            string value = req.get_param_value("value");
            store.insertKey(key, value);
            res.set_content("Key '" + key + "' inserted.", "text/plain");
        } else {
            res.status = 400;
            res.set_content("Bad Request: 'key' and 'value' parameters are required.", "text/plain");
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;
        cout << "[RESPONSE] " << req.method << " " << req.path << " - Status: " << res.status << " - Duration: " << duration.count() << " ms" << endl;
    });

    // Endpoint for retrieving a value by key
    svr.Get(R"(/get/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        auto start = chrono::high_resolution_clock::now();
        cout << "[REQUEST] " << req.method << " " << req.path << endl;

        string key = req.matches[1];
        string value = store.getKey(key);
        if (value == "Key not found.") {
            res.status = 404;
        }
        res.set_content(value, "text/plain");

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;
        cout << "[RESPONSE] " << req.method << " " << req.path << " - Status: " << res.status << " - Duration: " << duration.count() << " ms" << endl;
    });

    // Endpoint for deleting a key
    svr.Delete(R"(/delete/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        auto start = chrono::high_resolution_clock::now();
        cout << "[REQUEST] " << req.method << " " << req.path << endl;

        string key = req.matches[1];
        store.deleteKey(key);
        res.set_content("Key '" + key + "' deleted.", "text/plain");

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> duration = end - start;
        cout << "[RESPONSE] " << req.method << " " << req.path << " - Status: " << res.status << " - Duration: " << duration.count() << " ms" << endl;
    });

    cout << "[INFO] Starting web server on http://localhost:8080" << endl;
    svr.listen("localhost", 8080);
}
