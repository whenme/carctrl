// SPDX-License-Identifier: GPL-2.0
#include <spdlog/easylog.hpp>
#include <opencv2/opencv.hpp>
#include <video/video_ctrl.hpp>
#include <vector>

VideoCtrl::VideoCtrl(asio::io_service& io_service):
  m_videoThread("video thread", IoThread::ThreadPriorityNormal, videoThreadFun, this)
{
    //check there is GUI backend or not
    Mat img = imread("lena.png");
    if (!img.empty()) {
        try {
            imshow("lena", img);
            destroyWindow("lena");
        }
        catch(...) {
            // No GUI backend...
            m_showVideo = false;
        }
    }

    m_videoThread.start();
}

VideoCtrl::~VideoCtrl()
{
    if (m_videoThread.getRunState())
        m_videoThread.stop();
}

void VideoCtrl::videoThreadFun(void *ctxt)
{
    VideoCtrl *obj = static_cast<VideoCtrl *>(ctxt);
    if (!obj->m_videoDev.getDeviceState())
        return;

    Mat frame, gray, edge;
    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto& video = obj->m_videoDev.getVideoCapture();
        video >> frame;
        if (frame.empty()) {
            easylog::warn("device read video error...");
            continue;
        }

        obj->showImage("capture", frame);
        cvtColor(frame, gray, COLOR_BGR2GRAY); //to gray
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
}

void VideoCtrl::showImage(std::string title, Mat& mat)
{
    if (m_showVideo) {
        imshow(title, mat);
    }
}
