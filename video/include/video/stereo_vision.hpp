// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <string>

class StereoVision
{
public:
    StereoVision();
    ~StereoVision() = default;

    // Load calibration parameters from YAML/XML file
    bool loadCalibration(const std::string& calibFile);

    // Compute rectification maps (call once after loadCalibration)
    void initRectifyMap(cv::Size imageSize);

    // Core: input left/right frames, output disparity and depth maps
    void computeDisparity(const cv::Mat& leftFrame, const cv::Mat& rightFrame,
                          cv::Mat& disparity, cv::Mat& depth);

    // Query distance at pixel (x, y) in meters, returns -1.0f if invalid
    float getDistance(const cv::Mat& depth, int x, int y);

    // Generate colorized disparity map for visualization
    void getDisparityColor(const cv::Mat& disparity, cv::Mat& colorMap);

    bool isReady() const { return m_calibLoaded && m_rectifyReady; }

private:
    // Calibration parameters
    cv::Mat m_cameraMatrixL, m_distCoeffsL;
    cv::Mat m_cameraMatrixR, m_distCoeffsR;
    cv::Mat m_R, m_T;

    // Rectification maps (precomputed)
    cv::Mat m_mapLx, m_mapLy;
    cv::Mat m_mapRx, m_mapRy;
    cv::Mat m_Q;  // disparity-to-3D reprojection matrix

    // SGBM stereo matcher
    cv::Ptr<cv::StereoSGBM> m_sgbm;

    bool m_calibLoaded  {false};
    bool m_rectifyReady {false};
};
