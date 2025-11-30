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

TEST(IPCUtilsTest, SerializeDeserializeSingleKeypointFloat) {
    std::vector<cv::KeyPoint> kps;
    kps.emplace_back(1.0f, 2.0f, 5.0f, 0.0f, 1.0f, 0, -1);

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

TEST(IPCUtilsTest, SerializeDeserializeMultipleKeypoints) {
    std::vector<cv::KeyPoint> kps;
    cv::Mat desc(3, 128, CV_32F); // typical SIFT descriptor

    for(int i=0; i<3; i++) {
        kps.emplace_back(float(i), float(i+1), 5.0f);
        for(int j=0;j<128;j++) desc.at<float>(i,j) = float(i+j)/100.0f;
    }

    auto blob = serialize_keypoints_and_descriptors(kps, desc);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    ASSERT_EQ(kps_out.size(), 3);
    ASSERT_EQ(desc_out.rows, 3);
    ASSERT_EQ(desc_out.cols, 128);
    EXPECT_FLOAT_EQ(desc_out.at<float>(2,127), 1.29f); // last element check
}

TEST(IPCUtilsTest, SerializeDeserializeEmptyBlob) {
    std::vector<cv::KeyPoint> kps_out;
    cv::Mat desc_out;
    auto [kp, desc] = deserialize_keypoints_and_descriptors(std::vector<uint8_t>{});
    EXPECT_TRUE(kp.empty());
    EXPECT_TRUE(desc.empty());
}

TEST(IPCUtilsTest, SerializeDeserializeNonFloatDescriptor) {
    std::vector<cv::KeyPoint> kps;
    kps.emplace_back(0.0f, 0.0f, 1.0f);

    cv::Mat desc(1,3,CV_8U);
    desc.at<uint8_t>(0,0)=1; desc.at<uint8_t>(0,1)=2; desc.at<uint8_t>(0,2)=3;

    auto blob = serialize_keypoints_and_descriptors(kps, desc);
    auto [kps_out, desc_out] = deserialize_keypoints_and_descriptors(blob);

    ASSERT_EQ(kps_out.size(), 1);
    ASSERT_EQ(desc_out.rows,1);
    ASSERT_EQ(desc_out.cols,3);
    EXPECT_EQ(desc_out.at<uint8_t>(0,2),3);
}
