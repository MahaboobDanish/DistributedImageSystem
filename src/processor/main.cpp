#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
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
    catch (const std::exception &e){ std::cerr << "Failed to load config: " << e.what() << "\n"; return -1; }

    int pull_port = cfg["processor"]["subscribe_port"];
    int push_port = cfg["processor"]["publish_port"];
    int sift_nfeatures = cfg["processor"].value("sift_nfeatures", 0);
    std::string log_dir = cfg["logging"]["log_folder"];
    DualLogger logger(log_dir + "/processor.log");

    logger.info("Processor STARTED. Listening on port " + std::to_string(pull_port) +
                " and publishing to port " + std::to_string(push_port), true, true);

    zmq::context_t ctx(1);
    zmq::socket_t pull_sock(ctx, zmq::socket_type::pull);
    pull_sock.connect("tcp://127.0.0.1:" + std::to_string(pull_port));
    logger.info("Processor PULL connected", true, true);

    zmq::socket_t push_sock(ctx, zmq::socket_type::push);
    push_sock.bind("tcp://127.0.0.1:" + std::to_string(push_port));
    logger.info("Processor PUSH bound", true, true);

    cv::Ptr<cv::SIFT> detector = cv::SIFT::create(sift_nfeatures);

    while(running){
        zmq::message_t meta_msg, img_msg;
        if(!pull_sock.recv(meta_msg, zmq::recv_flags::none)) continue;
        pull_sock.recv(img_msg, zmq::recv_flags::none);

        std::string meta_s(static_cast<char *>(meta_msg.data()), meta_msg.size());
        json meta = json::parse(meta_s);

        std::vector<uchar> buf((uchar *)img_msg.data(), (uchar *)img_msg.data() + img_msg.size());
        cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
        if(img.empty()) { logger.warn("Failed to decode image", true, true); continue; }

        std::vector<cv::KeyPoint> keypoints;
        cv::Mat descriptors;
        detector->detectAndCompute(img, cv::noArray(), keypoints, descriptors);
        meta["num_keypoints"] = static_cast<int>(keypoints.size());

        std::vector<uchar> outbuf;
        cv::imencode(".jpg", img, outbuf, {cv::IMWRITE_JPEG_QUALITY, 90});
        std::vector<uint8_t> kp_blob = serialize_keypoints_and_descriptors(keypoints, descriptors);

        zmq::message_t out_meta(meta.dump());
        zmq::message_t out_img(outbuf.data(), outbuf.size());
        zmq::message_t out_kp(kp_blob.data(), kp_blob.size());

        push_sock.send(out_meta, zmq::send_flags::sndmore);
        push_sock.send(out_img, zmq::send_flags::sndmore);
        push_sock.send(out_kp, zmq::send_flags::none);

        logger.info("Processed image seq=" + std::to_string(meta.value("seq",0)), false, true);
    }

    logger.info("Processor STOPPED", true, true);
    return 0;
}
