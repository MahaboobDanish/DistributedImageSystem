#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <filesystem>
#include <csignal>
#include "common/ipc_utils.hpp"
#include "common/dual_logger.hpp"

using json = nlohmann::json;
volatile bool running = true;
void sigint_handler(int) { running = false; }

json loadConfig(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Cannot open config file: " + path);
    json j; f >> j; return j;
}

int main() {
    signal(SIGINT, sigint_handler);

    json cfg;
    try { cfg = loadConfig("config/default_config.json"); }
    catch(const std::exception &e){ std::cerr << "Failed to load config: " << e.what() << "\n"; return -1; }

    int subscribe_port = cfg["logger"]["subscribe_port"];
    std::string db_path = cfg["logger"]["db_path"];
    std::string images_dir = cfg["logger"]["image_save_path"];
    std::string log_dir = cfg["logging"]["log_folder"];
    DualLogger logger(log_dir + "/logger.log");

    logger.info("Logger STARTED. Listening on port " + std::to_string(subscribe_port) +
                ", saving images to " + images_dir + ", DB: " + db_path, true, true);

    std::filesystem::create_directories(std::filesystem::path(db_path).parent_path());
    std::filesystem::create_directories(images_dir);
    std::filesystem::create_directories(log_dir);

    zmq::context_t ctx(1);
    zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
    pull_sock.connect("tcp://127.0.0.1:" + std::to_string(subscribe_port));
    logger.info("Logger connected to processor PUSH", true, true);

    sqlite3 *db = nullptr;
    if(sqlite3_open(db_path.c_str(), &db)) { logger.error("Can't open DB: " + db_path, true, true); return 1; }

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

    while(running){
        zmq::message_t meta_msg, img_msg, kp_msg;
        if(!pull_sock.recv(meta_msg, zmq::recv_flags::none)) continue;
        pull_sock.recv(img_msg, zmq::recv_flags::none);
        pull_sock.recv(kp_msg, zmq::recv_flags::none);

        std::string meta_s(static_cast<char*>(meta_msg.data()), meta_msg.size());
        json meta = json::parse(meta_s);
        std::string image_id = meta.value("image_id","unknown");
        int seq = meta.value("seq",0);
        int num_kp = meta.value("num_keypoints",0);

        std::string img_filename = images_dir + "/" + image_id + ".jpg";
        std::ofstream ofs(img_filename, std::ios::binary);
        ofs.write(static_cast<char*>(img_msg.data()), img_msg.size());

        const char* insert_sql = "INSERT OR REPLACE INTO images(id,seq,timestamp,path,num_keypoints,kp_blob) VALUES(?,?,?,?,?,?);";
        sqlite3_stmt* stmt = nullptr;
        if(sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK){
            sqlite3_bind_text(stmt,1,image_id.c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt,2,seq);
            sqlite3_bind_text(stmt,3,meta.value("timestamp","").c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt,4,img_filename.c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt,5,num_kp);
            if(kp_msg.size() > 0) sqlite3_bind_blob(stmt,6,kp_msg.data(),static_cast<int>(kp_msg.size()),SQLITE_TRANSIENT);
            else sqlite3_bind_null(stmt,6);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        } else logger.error("Failed to prepare insert statement", true, true);

        logger.info("Logged image: " + image_id + " seq=" + std::to_string(seq) + " keypoints=" + std::to_string(num_kp), false, true);
    }

    sqlite3_close(db);
    logger.info("Logger STOPPED", true, true);
    return 0;
}
