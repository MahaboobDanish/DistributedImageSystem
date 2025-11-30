#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <filesystem>
#include <random>
#include "common/ipc_utils.hpp"

namespace fs = std::filesystem;

static std::string getRandomImageFromFolder(const std::string& folder, int max_samples = 10) {
    std::vector<std::string> images;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        if (ext == ".jpg" || ext == ".png" || ext == ".jpeg" || ext == ".bmp") {
            images.push_back(entry.path().string());
            if ((int)images.size() >= max_samples)
                break;
        }
    }

    if (images.empty())
        return "";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, images.size() - 1);

    return images[dist(gen)];
}

// ---------- REAL END-TO-END IMAGE → SIFT → SERIAL → DESERIAL ----------
TEST(E2ETest, RealImageSIFTRoundTrip) {

    const std::string imageFolder = "../../underwater_images";

    std::string imagePath = getRandomImageFromFolder(imageFolder, 10);

    ASSERT_FALSE(imagePath.empty())
        << "No valid images found in underwater_images folder";

    cv::Mat img = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);

    ASSERT_FALSE(img.empty())
        << "Failed to load selected image: " << imagePath;

    auto sift = cv::SIFT::create();
    std::vector<cv::KeyPoint> kps_in;
    cv::Mat desc_in;

    sift->detectAndCompute(img, cv::noArray(), kps_in, desc_in);

    ASSERT_FALSE(desc_in.empty()) << "No descriptors extracted";
    ASSERT_FALSE(kps_in.empty())  << "No keypoints detected";

    auto blob = serialize_keypoints_and_descriptors(kps_in, desc_in);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    ASSERT_EQ(kps_out.size(), kps_in.size());
    ASSERT_EQ(desc_out.rows, desc_in.rows);
    ASSERT_EQ(desc_out.cols, desc_in.cols);

    EXPECT_NEAR(desc_in.at<float>(0,0), desc_out.at<float>(0,0), 1e-6);
}


// TEST(E2ETest, SimpleImageFlow) {
//     // Load test image
//     cv::Mat img = cv::imread("../../underwater_images/test1.jpg", cv::IMREAD_GRAYSCALE);
//     ASSERT_FALSE(img.empty()) << "Test image not found";

//     // Run SIFT
//     cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
//     std::vector<cv::KeyPoint> keypoints;
//     cv::Mat descriptors;
//     sift->detectAndCompute(img, cv::noArray(), keypoints, descriptors);

//     // Serialize
//     auto blob = serialize_keypoints_and_descriptors(keypoints, descriptors);

//     // Simulate sending via IPC (deserialize)
//     auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

//     // Validate
//     EXPECT_EQ(kps_out.size(), keypoints.size());
//     EXPECT_EQ(desc_out.rows, descriptors.rows);
//     EXPECT_EQ(desc_out.cols, descriptors.cols);
// }

TEST(E2ETest, EmptyImageFlow) {
    cv::Mat img;
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;

    auto blob = serialize_keypoints_and_descriptors(keypoints, descriptors);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    EXPECT_TRUE(kps_out.empty());
    EXPECT_TRUE(desc_out.empty());
}

TEST(E2ETest, MultipleImagesFlow) {
    for(int i=1;i<=3;i++) {
        std::string path = "../../underwater_images/test" + std::to_string(i) + ".jpg";
        cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
        if(img.empty()) continue;

        cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
        std::vector<cv::KeyPoint> kps;
        cv::Mat desc;
        sift->detectAndCompute(img, cv::noArray(), kps, desc);

        auto blob = serialize_keypoints_and_descriptors(kps, desc);
        auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

        EXPECT_EQ(kps_out.size(), kps.size());
        EXPECT_EQ(desc_out.rows, desc.rows);
        EXPECT_EQ(desc_out.cols, desc.cols);
    }
}

