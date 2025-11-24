#include <zmq.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include "common/ipc_utils.hpp"

using json = nlohmann::json;
volatile bool running = true;
void sigint_handler(int) {running = false;}

int main(){
    signal(SIGINT, sigint_handler);
    zmq::context_t ctx(1);
    zmq::socket_t pull_sock(ctx,zmq::socket_type::pull);
    pull_sock.connect("tcp://127.0.0.1:6001"); // Connect to processors Push

    // ensure that images dir exists

    std::string images_dir = "images";
    std::system(("mkdir -p "+ images_dir).c_str());

    // open sqlite db
    sqlite3 *db = nullptr;
    int rc = sqlite3_open("data_log.db",&db);
    if(rc){
        std::cerr << "Can't open DB\n";
        return 1;
    }

    // create table if not exists (simple)
    const char*create_sql = "CREATE TABLE IF NOT EXISTS images("
                            "id TEXT PRIMARY KEY, seq INTEGER, timestamp TEXT, path TEXT, num_keypoints INTEGER);";

    sqlite3_exec(db,create_sql,0,0,0);

    while(running){
        zmq::message_t meta_msg, img_msg, kp_msg;
        auto res = pull_sock.recv(meta_msg, zmq::recv_flags::none);
        if(!res) continue;
        pull_sock.recv(img_msg, zmq::recv_flags::none);
        pull_sock.recv(kp_msg,zmq::recv_flags::none);

        std::string meta_s(static_cast<char*>(meta_msg.data()), meta_msg.size());
        json meta = json::parse(meta_s);
        std::string image_id = meta.value("image_id", std::string("unknown"));
        int seq = meta.value("seq",0);
        int num_kp = meta.value("num_keypoints",0);

        // Save image to disk:
        std::string img_filename = "images/" + image_id + ".jpg";
        std::ofstream ofs(img_filename,std::ios::binary);
        ofs.write(static_cast<char*>(img_msg.data()), img_msg.size());
        ofs.close();

        // Store metadata in sqlite
        std::string insert = "INSERT OR REPLACE INTO images(id, seq, timestamp, path, num_keypoints) VALUES(?,?,?,?,?);";
        sqlite3_stmt* stmt = nullptr;
        if(sqlite3_prepare_v2(db,insert.c_str(),-1,&stmt, nullptr)==SQLITE_OK){
            sqlite3_bind_text(stmt, 1, image_id.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt,2,seq);
            sqlite3_bind_text(stmt,3,meta.value("timestamp","").c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt,4, img_filename.c_str(),-1,SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, num_kp);

            //bind blob
            if(kp_msg.size()>0){
                sqlite3_bind_blob(stmt, 6, kp_msg.data(), (int)kp_msg.size(), SQLITE_TRANSIENT);
            } else {
                sqlite3_bind_null(stmt,6);
            }
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        } else{
            std::cerr << "Failed to prepare insert stmt\n";
        }

        // unpack blob to verify number of keypoints
        
        // if(kp_msg.size()>0){
        //     std::vector<uint8_t> blob((uint8_t*)kp_msg.data(), (uint8_t*)kp_msg.data() + kp_msg.size());
        //     auto [kps, descriptors] = deserialize_keypoints_and_descriptors(blob);
        //     std::cout << "Logged image: " << image_id << "seq = " << seq << " kp = " << kps.size() << "\n";
        // }else {
        //     std::cout << "Logged image: " << image_id << " seq = " << seq <<" kp = " << num_kp << "\n";
        // }
        
    }

    sqlite3_close(db);
    std::cout << "Logger existing\n";
    return 0;
}