// SPDX-License-Identifier: GPL-2.0

#include <xapi/easylog.hpp>
#include <video/stereo_vision.hpp>
#include <opencv2/imgproc.hpp>

StereoVision::StereoVision()
{
    // Create SGBM matcher with default parameters
    int numDisparities = 64;   // must be divisible by 16
    int blockSize = 9;         // odd number, typically 3~11

    m_sgbm = cv::StereoSGBM::create(
        0,                      // minDisparity
        numDisparities,         // numDisparities
        blockSize,              // blockSize
        8 * 1 * blockSize * blockSize,   // P1 (smoothness penalty)
        32 * 1 * blockSize * blockSize,  // P2 (smoothness penalty)
        1,                      // disp12MaxDiff
        63,                     // preFilterCap
        10,                     // uniquenessRatio
        100,                    // speckleWindowSize
        32,                     // speckleRange
        cv::StereoSGBM::MODE_SGBM_3WAY  // mode
    );
}

bool StereoVision::loadCalibration(const std::string& calibFile)
{
    cv::FileStorage fs(calibFile, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        ctrllog::warn("stereo: failed to open calibration file: {}", calibFile);
        return false;
    }

    fs["cameraMatrixL"] >> m_cameraMatrixL;
    fs["distCoeffsL"]   >> m_distCoeffsL;
    fs["cameraMatrixR"] >> m_cameraMatrixR;
    fs["distCoeffsR"]   >> m_distCoeffsR;
    fs["R"]             >> m_R;
    fs["T"]             >> m_T;
    fs.release();

    if (m_cameraMatrixL.empty() || m_cameraMatrixR.empty() ||
        m_distCoeffsL.empty()   || m_distCoeffsR.empty()   ||
        m_R.empty()             || m_T.empty()) {
        ctrllog::warn("stereo: calibration file missing required parameters");
        m_calibLoaded = false;
        return false;
    }

    ctrllog::info("stereo: calibration loaded from {}", calibFile);
    m_calibLoaded = true;
    return true;
}

void StereoVision::initRectifyMap(cv::Size imageSize)
{
    if (!m_calibLoaded) {
        ctrllog::warn("stereo: cannot init rectify map without calibration");
        return;
    }

    cv::Mat R1, R2, P1, P2;
    cv::stereoRectify(
        m_cameraMatrixL, m_distCoeffsL,
        m_cameraMatrixR, m_distCoeffsR,
        imageSize, m_R, m_T,
        R1, R2, P1, P2, m_Q,
        cv::CALIB_ZERO_DISPARITY, 0, imageSize
    );

    cv::initUndistortRectifyMap(m_cameraMatrixL, m_distCoeffsL, R1, P1,
                                imageSize, CV_32FC1, m_mapLx, m_mapLy);
    cv::initUndistortRectifyMap(m_cameraMatrixR, m_distCoeffsR, R2, P2,
                                imageSize, CV_32FC1, m_mapRx, m_mapRy);

    m_rectifyReady = true;
    ctrllog::info("stereo: rectification maps initialized ({}x{})", imageSize.width, imageSize.height);
}

void StereoVision::rectifyPair(const cv::Mat& leftFrame, const cv::Mat& rightFrame,
                               cv::Mat& rectL, cv::Mat& rectR)
{
    if (!isReady()) {
        rectL.release();
        rectR.release();
        return;
    }

    cv::remap(leftFrame, rectL, m_mapLx, m_mapLy, cv::INTER_LINEAR);
    cv::remap(rightFrame, rectR, m_mapRx, m_mapRy, cv::INTER_LINEAR);
}

void StereoVision::computeDisparityFromRectified(const cv::Mat& rectL, const cv::Mat& rectR,
                                                 cv::Mat& disparity, cv::Mat& depth)
{
    if (!isReady() || rectL.empty() || rectR.empty()) {
        disparity.release();
        depth.release();
        return;
    }

    cv::Mat grayL, grayR;
    if (rectL.channels() == 3) {
        cv::cvtColor(rectL, grayL, cv::COLOR_BGR2GRAY);
        cv::cvtColor(rectR, grayR, cv::COLOR_BGR2GRAY);
    } else {
        grayL = rectL;
        grayR = rectR;
    }

    cv::Mat disp16;
    m_sgbm->compute(grayL, grayR, disp16);
    disp16.convertTo(disparity, CV_32F, 1.0 / 16.0);
    cv::reprojectImageTo3D(disparity, depth, m_Q, true);
}

void StereoVision::computeDisparity(const cv::Mat& leftFrame, const cv::Mat& rightFrame,
                                    cv::Mat& disparity, cv::Mat& depth)
{
    cv::Mat rectL, rectR;
    rectifyPair(leftFrame, rightFrame, rectL, rectR);
    computeDisparityFromRectified(rectL, rectR, disparity, depth);
}

float StereoVision::getDistance(const cv::Mat& depth, int x, int y)
{
    if (depth.empty() || x < 0 || y < 0 ||
        x >= depth.cols || y >= depth.rows) {
        return -1.0f;
    }

    cv::Vec3f point = depth.at<cv::Vec3f>(y, x);
    float z = point[2];

    // Filter out invalid depth values
    if (z <= 0 || z > 10000.0f) {
        return -1.0f;
    }

    return z;
}

void StereoVision::getDisparityColor(const cv::Mat& disparity, cv::Mat& colorMap)
{
    if (disparity.empty()) {
        return;
    }

    // Normalize to 0-255 for visualization
    cv::Mat disp8;
    double minVal, maxVal;
    cv::minMaxLoc(disparity, &minVal, &maxVal);
    if (maxVal - minVal > 0) {
        disparity.convertTo(disp8, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    } else {
        disparity.convertTo(disp8, CV_8U);
    }

    cv::applyColorMap(disp8, colorMap, cv::COLORMAP_JET);
}
