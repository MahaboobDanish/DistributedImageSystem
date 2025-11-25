#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include "common/ipc_utils.hpp"

TEST(E2ETest, SimpleImageFlow) {
    // Load test image
    cv::Mat img = cv::imread("../../underwater_images/test1.jpg");
    ASSERT_FALSE(img.empty()) << "Test image not found";

    // Serialize
    auto blob = serialize_keypoints_and_descriptors({}, cv::Mat());

    // Simulate sending via IPC (here we just deserialize to validate flow)
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    // Validate
    EXPECT_EQ(kps_out.size(), 0);
    EXPECT_TRUE(desc_out.empty());
}
