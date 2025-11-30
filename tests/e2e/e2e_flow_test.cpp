#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <thread>
#include <chrono>
#include "common/ipc_utils.hpp"

TEST(E2ETest, SimpleImageFlow) {
    // Load test image
    cv::Mat img = cv::imread("../../underwater_images/test1.jpg", cv::IMREAD_GRAYSCALE);
    ASSERT_FALSE(img.empty()) << "Test image not found";

    // Run SIFT
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    sift->detectAndCompute(img, cv::noArray(), keypoints, descriptors);

    // Serialize
    auto blob = serialize_keypoints_and_descriptors(keypoints, descriptors);

    // Simulate sending via IPC (deserialize)
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    // Validate
    EXPECT_EQ(kps_out.size(), keypoints.size());
    EXPECT_EQ(desc_out.rows, descriptors.rows);
    EXPECT_EQ(desc_out.cols, descriptors.cols);
}

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

