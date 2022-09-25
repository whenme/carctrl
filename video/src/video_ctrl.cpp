// SPDX-License-Identifier: GPL-2.0
#include <opencv2/opencv.hpp>
#include <video/video_ctrl.hpp>
#include <vector>

VideoCtrl::VideoCtrl(asio::io_service& io_service):
  m_ios(io_service),
  m_timer(io_service, timerCallback, this, true),
  m_showVideo(true)
{
    m_timer.start(100);
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

    obj->showImage("capture", resizeFrame);
    Mat gray, thres, edge;
    cvtColor(resizeFrame, gray, COLOR_BGR2GRAY); //to gray
    GaussianBlur(gray, gray, Size(5, 5), 0);  //gauss filter
    obj->showImage("gray", gray);

    Canny(gray, edge, 40, 100, 3, false); //edge detection
    //threshold(edge, edge, 170, 255, THRESH_BINARY);

    std::vector<Vec4i> lineP1;
    HoughLinesP(edge, lineP1, 1, CV_PI/180, 90, 30, 40); //100/90
    for (size_t i = 0; i < lineP1.size(); i++) { //draw line
        line(edge, Point(lineP1[i][0], lineP1[i][1]), Point(lineP1[i][2], lineP1[i][3]), Scalar(255), 3);
    }
    obj->showImage("edge", edge);

    waitKey(1000 / video.get(CAP_PROP_FPS));
}

void VideoCtrl::showImage(std::string title, Mat& mat)
{
    if (m_showVideo) {
        imshow(title, mat);
    }
}
