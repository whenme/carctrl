// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <mutex>
#include <vector>
#include <opencv2/opencv.hpp>
#include <xapi/iotimer.hpp>
#include <xapi/cmn_thread.hpp>
#include <video/video_device.hpp>
#include <video/stereo_vision.hpp>

class VideoCtrl
{
public:
    VideoCtrl(asio::io_context& ioContext);
    virtual ~VideoCtrl();

    bool getWebFrame(int32_t camera, std::vector<uint8_t>& out);
    int32_t getWebCameraCount();
    bool isCameraExist(int32_t camera);

private:
    void        showImage(std::string title, Mat& mat);
    void        updateWebFrame(int32_t camera, const cv::Mat& mat);
    static void videoThreadFun(void *ctxt);

    static constexpr int32_t video_dev_num = 2;
    static constexpr const char* stereo_calib_file = "stereo_calib.yaml";

    cmn::CmnThread m_videoThread;
    VideoDevice*   m_videoDev[video_dev_num];
    bool           m_showVideo   { true };
    bool           m_stereoMode  { false };
    int32_t        m_videoDevNum {0};
    StereoVision   m_stereoVision;
    std::mutex     m_webFrameMutex;
    std::vector<std::vector<uint8_t>> m_webJpeg;
    std::vector<bool> m_hasWebFrame;
};
