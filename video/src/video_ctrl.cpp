// SPDX-License-Identifier: GPL-2.0
#include <opencv2/opencv.hpp>
#include <video/video_ctrl.hpp>

VideoCtrl::VideoCtrl(asio::io_service& io_service):
  m_ios(io_service),
  m_timer(io_service, timerCallback, this, true)
{
    m_timer.start(1000);
}

VideoCtrl::~VideoCtrl()
{
    m_timer.stop();
}

void VideoCtrl::timerCallback(const asio::error_code &e, void *ctxt)
{
    VideoCtrl *obj = static_cast<VideoCtrl *>(ctxt);
    bool videoState = obj->m_videoDev.getDeviceState();
    if (!videoState) {
        std::cout << "video device error..." << std::endl;
        return;
    }

    Mat frame, resizeFrame;
    auto& video = obj->m_videoDev.getVideoCapture();
    video >> frame;
    if (frame.empty()) {
        std::cout << "device read video error..." << std::endl;
        return;
    }

    resize(frame, resizeFrame, Size(1024, 680), INTER_NEAREST);
    //imshow("video", resizeFrame);
    waitKey(1000 / video.get(CAP_PROP_FPS));
}