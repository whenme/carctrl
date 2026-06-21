// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

/**
 * Runs MobileNet-SSD (PASCAL VOC) on rectified left/right images and logs
 * class names (Chinese) for objects that appear in both views with consistent
 * epipolar geometry. Model files are optional; set CARCTRL_SSD_PROTO and
 * CARCTRL_SSD_MODEL or place MobileNetSSD_deploy.{prototxt,caffemodel} in cwd.
 */
class StereoObjectMatch
{
public:
    StereoObjectMatch();

    /** Throttled; safe to call every frame when stereo is active. */
    void tryDetectAndLog(const cv::Mat& rectifiedLeftBgr, const cv::Mat& rectifiedRightBgr);

private:
    bool tryLoadNet();

    cv::dnn::Net m_net;
    bool           m_ready {false};
    bool           m_warned {false};
    int            m_frameCounter {0};
    std::chrono::steady_clock::time_point m_lastLog {};
};
