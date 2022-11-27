// SPDX-License-Identifier: GPL-2.0
#ifndef __VIDEO_CTRL_HPP__
#define __VIDEO_CTRL_HPP__

#include <opencv2/opencv.hpp>
#include <ioapi/iotimer.hpp>
#include <ioapi/iothread.hpp>
#include <video/video_device.hpp>

class VideoCtrl
{
public:
    VideoCtrl(asio::io_service& io_service);
    virtual ~VideoCtrl();

private:
    IoThread    m_videoThread;
    VideoDevice m_videoDev;
    bool        m_showVideo { true };

    void        showImage(std::string title, Mat& mat);
    static void videoThreadFun(void *ctxt);
};

#endif