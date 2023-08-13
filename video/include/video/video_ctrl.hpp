// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <opencv2/opencv.hpp>
#include <ioapi/iotimer.hpp>
#include <ioapi/iothread.hpp>
#include <video/video_device.hpp>

class VideoCtrl
{
public:
    VideoCtrl(asio::io_context& ioContext);
    virtual ~VideoCtrl();

private:
    IoThread    m_videoThread;
    VideoDevice m_videoDev;
    bool        m_showVideo { true };

    void        showImage(std::string title, Mat& mat);
    static void videoThreadFun(void *ctxt);
};
