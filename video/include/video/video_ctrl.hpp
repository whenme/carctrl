// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <opencv2/opencv.hpp>
#include <xapi/iotimer.hpp>
#include <xapi/cmn_thread.hpp>
#include <video/video_device.hpp>

class VideoCtrl
{
public:
    VideoCtrl(asio::io_context& ioContext);
    virtual ~VideoCtrl();

private:
    cmn::CmnThread m_videoThread;
    VideoDevice    m_videoDev;
    bool           m_showVideo { false };

    void        showImage(std::string title, Mat& mat);
    static void videoThreadFun(void *ctxt);
};
