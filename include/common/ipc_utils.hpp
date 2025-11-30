#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <random>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <vector>
#include <cstdint>
#include <cstring>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>

namespace config {
    nlohmann::json loadConfig(const std::string &path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            throw std::runtime_error("Cannot open config file: " + path);
        }
        nlohmann::json j;
        f >> j;
        return j;
    }
}

inline std::string now_iso8601(){
    using namespace std::chrono;
    auto t = system_clock::now();
    auto s = system_clock::to_time_t(t);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&s), "%F%TZ");
    return oss.str();
}

inline std::string gen_simple_id() {
    // Quickly generate a psudo Unique ID UUID using timesptamp
    static std::mt19937_64 rng(std::random_device{}());
    uint64_t r = rng();
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss <<std::hex << now << "- " << r;
    return oss.str();
}

/*
uint32_t N => NUmber of keypoints
uint32_t D => Descriptor lenght per keypoints
uint32_t desc_type => (0=float32 descriptor entries, 1 = uint8_t descriptor entries)

*/

inline std::vector<uint8_t> serialize_keypoints_and_descriptors(
    const std::vector<cv::KeyPoint>& kps,
    const cv::Mat& descriptors){
        std::vector<uint8_t> out;
        uint32_t N = static_cast<uint32_t>(kps.size());
        uint32_t D = 0;
        uint8_t desc_type = 0; // 0=> float32, 1=>uint8
        if(!descriptors.empty()){
            D = static_cast<uint32_t>(descriptors.cols);
            if(descriptors.type()==CV_32F) desc_type=0;
            else desc_type = 1; // CV_8U or others -> store as bytes
        }

        // reserve approximate size
        size_t per_kp_meta = 4*5 + 4 + 4; //x,y,size, angle, response (4 bytes each) + octave (int32) + calss_id(int32)
        size_t desc_bytes_per = (desc_type==0 ? 4u:1u)*(D?D:0u);
        out.reserve(8 + N * (per_kp_meta + desc_bytes_per));

        // append N (uint32)
        uint32_t tmp32 = N;
        out.insert(out.end(), reinterpret_cast<uint8_t*>(&tmp32), reinterpret_cast<uint8_t*>(&tmp32) + sizeof(uint32_t));

        // append D
        tmp32 = D;
        out.insert(out.end(),reinterpret_cast<uint8_t*>(&tmp32), reinterpret_cast<uint8_t*>(&tmp32) + sizeof(uint32_t));

        // append desc_type
        out.push_back(desc_type);

        //for each keypoint
        for(uint32_t i=0;i<N;i++){
            const cv::KeyPoint &kp = kps[i];
            float fx = kp.pt.x;
            float fy = kp.pt.y;
            float fsize = kp.size;
            float fangle = kp.angle;
            float fresponse = kp.response;
            int32_t octave = kp.octave;
            int32_t class_id = kp.class_id;

            out.insert(out.end(), reinterpret_cast<uint8_t*>(&fx), reinterpret_cast<uint8_t*>(&fx) + sizeof(float));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&fy), reinterpret_cast<uint8_t*>(&fy) + sizeof(float));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&fsize), reinterpret_cast<uint8_t*>(&fsize) + sizeof(float));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&fangle), reinterpret_cast<uint8_t*>(&fangle) + sizeof(float));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&fresponse), reinterpret_cast<uint8_t*>(&fresponse) + sizeof(float));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&octave), reinterpret_cast<uint8_t*>(&octave) + sizeof(int32_t));
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&class_id), reinterpret_cast<uint8_t*>(&class_id) + sizeof(int32_t));

            //descriptors
            if(D>0){
                if(desc_type==0){
                    // float descriptors expected (CV_32F)
                    const float* row = descriptors.ptr<float>(i);
                    for(uint32_t di=0; di<D; ++di){
                        float v = row[di];
                        out.insert(out.end(), reinterpret_cast<uint8_t*>(&v), reinterpret_cast<uint8_t*>(&v) + sizeof(float));
                    }
                }else {
                    const uint8_t* row = descriptors.ptr<uint8_t>(i);
                    out.insert(out.end(), row, row + D);
                }
            }

        }

    return out;
}

// minimal deserializer: returns pair of keypoints vector and descriptors Mat
inline std::pair<std::vector<cv::KeyPoint>, cv::Mat> deserialize_keypoints_and_descriptors(const std::vector<uint8_t>& blob){
    const uint8_t* p = blob.data();
    size_t bytes = blob.size();
    size_t offset = 0;
    if(bytes < 9) return {{}, cv::Mat()}; // too small

    auto read_u32 = [&](uint32_t &out)->bool{
        if(offset + 4 > bytes) return false;
        std::memcpy(&out, p + offset, 4);
        offset += 4;
        return true;
    };
    auto read_i32 = [&](int32_t &out)->bool{
        if(offset + 4 > bytes) return false;
        std::memcpy(&out, p + offset, 4);
        offset += 4;
        return true;
    };
    auto read_f32 = [&](float &out)->bool{
        if(offset + 4 > bytes) return false;
        std::memcpy(&out, p + offset, 4);
        offset += 4;
        return true;
    };

    uint32_t N=0, D=0;
    if(!read_u32(N)) return {{}, cv::Mat()};
    if(!read_u32(D)) return {{}, cv::Mat()};
    if(offset + 1 > bytes) return {{}, cv::Mat()};
    uint8_t desc_type = p[offset]; offset += 1;

    std::vector<cv::KeyPoint> kps;
    kps.reserve(N);

    // prepare descriptor matrix
    cv::Mat descriptors;
    if(N>0 && D>0){
        if(desc_type==0) descriptors.create((int)N, (int)D, CV_32F);
        else descriptors.create((int)N, (int)D, CV_8U);
    }

    for(uint32_t i=0;i<N;++i){
        float x,y,size,angle,response;
        int32_t octave,class_id;
        if(!read_f32(x)) break;
        if(!read_f32(y)) break;
        if(!read_f32(size)) break;
        if(!read_f32(angle)) break;
        if(!read_f32(response)) break;
        if(!read_i32(octave)) break;
        if(!read_i32(class_id)) break;

        cv::KeyPoint kp(cv::Point2f(x,y), size, angle, response, octave, class_id);
        kps.push_back(kp);

        if(D>0){
            if(desc_type==0){
                // float
                float* row = descriptors.ptr<float>(i);
                for(uint32_t di=0; di<D; ++di){
                    float v;
                    if(!read_f32(v)) break;
                    row[di] = v;
                }
            } else {
                uint8_t* row = descriptors.ptr<uint8_t>(i);
                if(offset + D > bytes) break;
                std::memcpy(row, p + offset, D);
                offset += D;
            }
        }
    }

    return {kps, descriptors};
}

