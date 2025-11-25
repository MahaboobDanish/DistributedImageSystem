// src/logger/main.cpp
#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <filesystem>
#include <csignal>
#include "common/ipc_utils.hpp"

using json = nlohmann::json;
volatile bool running = true;
void sigint_handler(int) { running = false; }

json loadConfig(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Cannot open config file: " + path);
    json j;
    f >> j;
    return j;
}

int main() {
    signal(SIGINT, sigint_handler);

    // Load config (project-root relative)
    json cfg;
    try {
        cfg = loadConfig("config/default_config.json");
    } catch (const std::exception &e) {
        std::cerr << "Failed to load config: " << e.what() << "\n";
        return -1;
    }

    int subscribe_port = cfg["logger"]["subscribe_port"];
    std::string db_path = cfg["logger"]["db_path"];
    std::string images_dir = cfg["logger"]["image_save_path"];
    std::string log_dir = cfg["logging"]["log_folder"];

    // Ensure directories exist (relative to project root)
    std::filesystem::create_directories(std::filesystem::path(db_path).parent_path());
    std::filesystem::create_directories(images_dir);
    std::filesystem::create_directories(log_dir);

    // ZeroMQ: connect to processor PUSH (processor binds)
    zmq::context_t ctx(1);
    zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
    std::string pull_address = "tcp://127.0.0.1:" + std::to_string(subscribe_port);
    pull_sock.connect(pull_address);
    std::cout << "Logger connected to: " << pull_address << "\n";

    // SQLite DB
    sqlite3 *db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db)) {
        std::cerr << "Can't open DB at " << db_path << "\n";
        return 1;
    }

    const char* create_sql = R"(
        CREATE TABLE IF NOT EXISTS images(
            id TEXT PRIMARY KEY,
            seq INTEGER,
            timestamp TEXT,
            path TEXT,
            num_keypoints INTEGER,
            kp_blob BLOB
        );
    )";
    sqlite3_exec(db, create_sql, nullptr, nullptr, nullptr);

    // Main receive loop: expect 3-part messages: meta, image, kp_blob
    while (running) {
        zmq::message_t meta_msg, img_msg, kp_msg;

        auto res = pull_sock.recv(meta_msg, zmq::recv_flags::none);
        if (!res) continue;
        pull_sock.recv(img_msg, zmq::recv_flags::none);
        pull_sock.recv(kp_msg, zmq::recv_flags::none);

        std::string meta_s(static_cast<char*>(meta_msg.data()), meta_msg.size());
        json meta = json::parse(meta_s);
        std::string image_id = meta.value("image_id", std::string("unknown"));
        int seq = meta.value("seq", 0);
        int num_kp = meta.value("num_keypoints", 0);

        // Save image to disk
        std::string img_filename = images_dir + "/" + image_id + ".jpg";
        {
            std::ofstream ofs(img_filename, std::ios::binary);
            ofs.write(static_cast<char*>(img_msg.data()), img_msg.size());
        }

        // Insert metadata and kp_blob into DB
        const char* insert_sql = "INSERT OR REPLACE INTO images(id, seq, timestamp, path, num_keypoints, kp_blob) VALUES(?,?,?,?,?,?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, image_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, seq);
            sqlite3_bind_text(stmt, 3, meta.value("timestamp","").c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, img_filename.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, num_kp);
            if(kp_msg.size() > 0)
                sqlite3_bind_blob(stmt, 6, kp_msg.data(), static_cast<int>(kp_msg.size()), SQLITE_TRANSIENT);
            else
                sqlite3_bind_null(stmt, 6);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        } else {
            std::cerr << "Failed to prepare insert statement\n";
        }

        std::cout << "Logged image: " << image_id << " seq=" << seq << " keypoints=" << num_kp << "\n";
    }

    sqlite3_close(db);
    std::cout << "Logger exiting\n";
    return 0;
}


// #include <zmq.hpp>
// #include <iostream>
// #include <fstream>
// #include <nlohmann/json.hpp>
// #include <sqlite3.h>
// #include <filesystem>
// #include <csignal>
// #include "common/ipc_utils.hpp"

// using json = nlohmann::json;
// volatile bool running = true;
// void sigint_handler(int) { running = false; }

// int main() {
//     signal(SIGINT, sigint_handler);

//     // ---------------------------
//     // Load config
//     // ---------------------------
//     json cfg;
//     try {
//         // std::ifstream f("../config/default_config.json");
//         std::ifstream f("config/default_config.json");
//         if (!f.is_open()) {
//             std::cerr << "Cannot open config file\n";
//             return 1;
//         }
//         f >> cfg;
//     } catch (std::exception &e) {
//         std::cerr << "Failed to parse config: " << e.what() << "\n";
//         return 1;
//     }

//     int pull_port = cfg["logger"]["subscribe_port"];
//     std::string db_path = cfg["logger"]["db_path"];
//     std::string images_dir = cfg["logger"]["image_save_path"];
//     std::string log_dir = cfg["logging"]["log_folder"];

//     // Ensure directories exist
//     std::filesystem::create_directories(std::filesystem::path(db_path).parent_path());
//     std::filesystem::create_directories(images_dir);
//     std::filesystem::create_directories(log_dir);

//     // ---------------------------
//     // Setup ZMQ
//     // ---------------------------
//     zmq::context_t ctx(1);
//     zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
//     std::string pull_address = "tcp://127.0.0.1:" + std::to_string(pull_port);
//     pull_sock.connect(pull_address);
//     std::cout << "Logger connected to: " << pull_address << "\n";

//     // ---------------------------
//     // Setup SQLite DB
//     // ---------------------------
//     sqlite3 *db = nullptr;
//     if (sqlite3_open(db_path.c_str(), &db)) {
//         std::cerr << "Can't open DB at " << db_path << "\n";
//         return 1;
//     }

//     const char* create_sql = R"(
//         CREATE TABLE IF NOT EXISTS images(
//             id TEXT PRIMARY KEY,
//             seq INTEGER,
//             timestamp TEXT,
//             path TEXT,
//             num_keypoints INTEGER
//         );
//     )";
//     sqlite3_exec(db, create_sql, nullptr, nullptr, nullptr);

//     // ---------------------------
//     // Main loop
//     // ---------------------------
//     while (running) {
//         zmq::message_t meta_msg, img_msg, kp_msg;

//         auto res = pull_sock.recv(meta_msg, zmq::recv_flags::none);
//         if (!res) continue;
//         pull_sock.recv(img_msg, zmq::recv_flags::none);
//         pull_sock.recv(kp_msg, zmq::recv_flags::none);

//         std::string meta_s(static_cast<char*>(meta_msg.data()), meta_msg.size());
//         json meta = json::parse(meta_s);
//         std::string image_id = meta.value("image_id", std::string("unknown"));
//         int seq = meta.value("seq", 0);
//         int num_kp = meta.value("num_keypoints", 0);

//         // Save image to disk
//         std::string img_filename = images_dir + "/" + image_id + ".jpg";
//         std::ofstream ofs(img_filename, std::ios::binary);
//         ofs.write(static_cast<char*>(img_msg.data()), img_msg.size());
//         ofs.close();

//         // Store metadata in SQLite
//         std::string insert = "INSERT OR REPLACE INTO images(id, seq, timestamp, path, num_keypoints) VALUES(?,?,?,?,?);";
//         sqlite3_stmt* stmt = nullptr;
//         if (sqlite3_prepare_v2(db, insert.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
//             sqlite3_bind_text(stmt, 1, image_id.c_str(), -1, SQLITE_TRANSIENT);
//             sqlite3_bind_int(stmt, 2, seq);
//             sqlite3_bind_text(stmt, 3, meta.value("timestamp", "").c_str(), -1, SQLITE_TRANSIENT);
//             sqlite3_bind_text(stmt, 4, img_filename.c_str(), -1, SQLITE_TRANSIENT);
//             sqlite3_bind_int(stmt, 5, num_kp);

//             sqlite3_step(stmt);
//             sqlite3_finalize(stmt);
//         } else {
//             std::cerr << "Failed to prepare insert stmt\n";
//         }

//         std::cout << "Logged image: " << image_id << " seq=" << seq << " keypoints=" << num_kp << "\n";
//     }

//     sqlite3_close(db);
//     std::cout << "Logger exiting\n";
//     return 0;
// }
