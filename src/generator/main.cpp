#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>
#include "common/ipc_utils.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

volatile bool running = true;

void sigint_handler(int){ running = false;}

int main(int argc, char** argv){
    if(argc<2){
        std::cerr << "Usage: generator < images-folder>\n";
        return 1;
    }

    std::string folder = argv[1];

    signal(SIGINT, sigint_handler);
    zmq::context_t ctx(1);
    zmq::socket_t push_sock(ctx, zmq::socket_type::push);
    // push_sock.bind("tcp://127.0.0.1:557"); // Generator PUSH -> Processor
    push_sock.bind("tcp://127.0.0.1:6000");

    std::vector<fs::path> imgs;
    for(auto &p : fs::directory_iterator(folder)){
        if(!p.is_regular_file()) continue;
        imgs.push_back(p.path());
    }
    if(imgs.empty()){
        std::cerr << "No IMages found in folder\n";
    }

    size_t idx = 0;
    while(running){
        auto path = imgs[idx%imgs.size()];
        idx++;

        cv::Mat image = cv::imread(path.string(), cv::IMREAD_COLOR);
        if(image.empty()){
            std::cerr << "Failed to read " << path <<"\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // encode as JPEG
        std::vector<uchar> buf;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY,90};
        cv::imencode(".jpg",image,buf,params);

        //Metadata
        json meta;
        meta["image_id"] = gen_simple_id();
        meta["timestamp"] = now_iso8601();
        meta["width"] = image.cols;
        meta["height"] = image.rows;
        meta["encoding"] = "jpg";
        meta["seq"] = (int)idx;

        zmq::message_t meta_msg(meta.dump());
        zmq::message_t img_msg(buf.data(), buf.size());

        // send multipart
        push_sock.send(meta_msg,zmq::send_flags::sndmore);
        push_sock.send(img_msg,zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Pacing
    }
    std::cout <<"Generator exiting\n";
    return 0;
}
