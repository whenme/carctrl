// SPDX-License-Identifier: GPL-2.0
#ifndef __VIDEO_CTRL_HPP__
#define __VIDEO_CTRL_HPP__

#include <ioapi/iotimer.hpp>
#include <video/video_device.hpp>

class VideoCtrl
{
public:
    VideoCtrl(asio::io_service& io_service);
    virtual ~VideoCtrl();

private:
    asio::io_service& m_ios;
    VideoDevice       m_videoDev;
    IoTimer           m_timer;

    static void timerCallback(const asio::error_code &e, void *ctxt);
};

#endif