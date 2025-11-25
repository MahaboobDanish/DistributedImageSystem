#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include "common/ipc_utils.hpp"

TEST(IPCUtilsTest, SerializeDeserializeKeypointsEmpty) {
    std::vector<cv::KeyPoint> kps;
    cv::Mat desc;
    auto blob = serialize_keypoints_and_descriptors(kps, desc);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);
    EXPECT_EQ(kps_out.size(), 0);
    EXPECT_TRUE(desc_out.empty());
}

TEST(IPCUtilsTest, SerializeDeserializeKeypointsWithFloat) {
    // std::vector<cv::KeyPoint> kps;
    // kps.emplace_back(cv::Point2f(1.0f, 2.0f), 5.0f, 0.0f, 1.0f, 0, 0, 0);
    std::vector<cv::KeyPoint> kps;
    kps.emplace_back(1.0f, 2.0f, 5.0f, 0.0f, 1.0f, 0, -1);  // class_id = -1

    cv::Mat desc(1, 2, CV_32F);
    desc.at<float>(0,0) = 0.1f;
    desc.at<float>(0,1) = 0.2f;

    auto blob = serialize_keypoints_and_descriptors(kps, desc);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    ASSERT_EQ(kps_out.size(), 1);
    EXPECT_FLOAT_EQ(kps_out[0].pt.x, 1.0f);
    EXPECT_FLOAT_EQ(kps_out[0].pt.y, 2.0f);
    EXPECT_EQ(desc_out.rows, 1);
    EXPECT_EQ(desc_out.cols, 2);
    EXPECT_FLOAT_EQ(desc_out.at<float>(0,0), 0.1f);
    EXPECT_FLOAT_EQ(desc_out.at<float>(0,1), 0.2f);
}
