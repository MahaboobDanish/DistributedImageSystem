#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include "common/ipc_utils.hpp"
#include "common/dual_logger.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
volatile bool running = true;
void sigint_handler(int){ running = false; }

int main(int argc, char** argv){
    signal(SIGINT, sigint_handler);

    // Load configuration
    json cfg;
    try {
        cfg = config::loadConfig("config/default_config.json");
    } catch(const std::exception& e){
        std::cerr << "Error loading config: " << e.what() << "\n";
        return 1;
    }

    // Determine image folder
    std::string folder = (argc > 1) ? argv[1] : cfg["generator"]["image_folder"];
    fs::path folder_path(folder);
    if(!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
        std::cerr << "No images found in folder: " << folder_path << "\n";
        return 1;
    }

    int port = cfg["generator"]["publish_port"];
    bool loop_images = cfg["generator"].value("loop_images", true);
    int sleep_ms = cfg["generator"].value("sleep_ms", 200);
    std::string log_dir = cfg["logging"]["log_folder"];
    DualLogger logger(log_dir + "/generator.log");

    logger.info("Generator STARTED. Publishing images from: " + folder, true, true);

    // Setup ZMQ PUSH socket
    zmq::context_t ctx(1);
    zmq::socket_t push_sock(ctx, zmq::socket_type::push);
    std::string address = "tcp://127.0.0.1:" + std::to_string(port);
    push_sock.bind(address);
    logger.info("Generator bound to " + address, true, true);

    // Load images
    std::vector<fs::path> imgs;
    for(auto &p : fs::directory_iterator(folder)) if(p.is_regular_file()) imgs.push_back(p.path());
    if(imgs.empty()) {
        logger.error("No images found in folder: " + folder, true, true);
        return 1;
    }

    size_t idx = 0;
    while(running){
        auto path = imgs[idx % imgs.size()];
        idx++;
        cv::Mat image = cv::imread(path.string(), cv::IMREAD_COLOR);
        if(image.empty()){
            logger.warn("Failed to read " + path.string(), true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<uchar> buf;
        cv::imencode(".jpg", image, buf, {cv::IMWRITE_JPEG_QUALITY, 90});

        json meta;
        meta["image_id"] = gen_simple_id();
        meta["timestamp"] = now_iso8601();
        meta["width"] = image.cols;
        meta["height"] = image.rows;
        meta["encoding"] = "jpg";
        meta["seq"] = static_cast<int>(idx);

        zmq::message_t meta_msg(meta.dump());
        zmq::message_t img_msg(buf.data(), buf.size());
        push_sock.send(meta_msg, zmq::send_flags::sndmore);
        push_sock.send(img_msg, zmq::send_flags::none);

        logger.info("Published image seq=" + std::to_string(idx), false, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        if(!loop_images && idx >= imgs.size()) break;
    }

    logger.info("Generator STOPPED", true, true);
    return 0;
}
