#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include "common/ipc_utils.hpp"

using json = nlohmann::json;
volatile bool running = true;
void sigint_handler(int){ running = false;}

int main(){
    signal(SIGINT, sigint_handler);
    zmq::context_t ctx(1);

    // PULL from generator
    zmq::socket_t pull_sock(ctx,zmq::socket_type::pull);
    pull_sock.connect("tcp://127.0.0.1:6000");

    // PUSH to logger
    zmq::socket_t push_sock(ctx,zmq::socket_type::push);
    push_sock.bind("tcp://127.0.0.1:6001");

    // createe SIFT detector
    cv::Ptr<cv::SIFT> detector = cv::SIFT::create();

    while(running){
        zmq::message_t meta_msg;
        zmq::message_t img_msg;

        // recv multipart (first meta then image), to block recv: can add poller/timeout
        auto res = pull_sock.recv(meta_msg, zmq::recv_flags::none);
        if(!res) continue;
        pull_sock.recv(img_msg, zmq::recv_flags::none);

        std::string meta_s(static_cast<char*>(meta_msg.data()), meta_msg.size());
        json meta = json::parse(meta_s);

        // DECODE Image
        std::vector<uchar> buf((uchar*)img_msg.data(), (uchar*)img_msg.data() + img_msg.size());
        cv::Mat img = cv::imdecode(buf,cv::IMREAD_COLOR);
        
        if(img.empty()){
            std::cerr << "Processor: failed to decade image\n";
            continue;
        }

        // TODO: SIFT Extraction

        // SIFT Detection
        
        /*
        detector->detectAndCompute gives 
        descriptors as CV_32F for SIFT in OpenCV; 
        serializer handles CV_32F and CV_8U.
        */
        std::vector<cv::KeyPoint> keypoints;
        cv::Mat descriptors;
        detector -> detectAndCompute(img, cv::noArray(), keypoints, descriptors);


        // // For time being Let us say mock 0 keypoints
        // meta["num_keypoints"] = 0;

        //fill metadata
        meta["num_keypoints"] = static_cast<int>(keypoints.size());


        // reEncode image to send
        std::vector<uchar> outbuf;
        cv::imencode(".jpg",img, outbuf,{cv::IMWRITE_JPEG_QUALITY,90});

        // // EMPTY Keypoint blod for now
        // std::vector<uint8_t> kp_blob;
        // zmq::message_t kp_msg(kp_blob.data(),kp_blob.size());

        // serialize keypoints+descriptors
        std::vector<uint8_t> kp_blob = serialize_keypoints_and_descriptors(keypoints,descriptors);

        // prepare zmq messages
        zmq::message_t out_meta(meta.dump());
        zmq::message_t out_img(outbuf.data(), outbuf.size());
        zmq::message_t out_kp(kp_blob.data(), kp_blob.size());
    
        // send mu;tipart: meta, image, kp_blob
        push_sock.send(out_meta, zmq::send_flags::sndmore);
        push_sock.send(out_img,zmq::send_flags::sndmore);
        push_sock.send(out_kp,zmq::send_flags::none);

        std::cout <<"Processed image seq= " << meta.value("seq",0) << "kps= " <<keypoints.size() << "\n";
    }
    std::cout << "Processor exiting\n";
    return 0;
}
