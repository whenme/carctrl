// SPDX-License-Identifier: GPL-2.0
#include <xapi/easylog.hpp>
#include <xapi/cmn_libfork.hpp>
#include <opencv2/opencv.hpp>
#include <video/video_ctrl.hpp>
#include <functional>
#include <vector>

VideoCtrl::VideoCtrl(asio::io_context& ioContext):
    m_videoThread("video thread", cmn::CmnThread::ThreadPriorityNormal, videoThreadFun, this),
    m_videoDev{nullptr, nullptr}
{
    //check there is GUI backend or not
    /*Mat img = imread("lena.png");
    if (!img.empty()) {
        try {
            imshow("lena", img);
            destroyWindow("lena");
        }
        catch(...) {
            // No GUI backend...
            m_showVideo = false;
        }
    }*/

    //find video device id. some video device begin from /dev/video4 as OPI3B
    int32_t idExist = 0;
    for (int32_t vid = 0; vid < video_dev_num; vid++) {
        m_videoDev[m_videoDevNum] = new VideoDevice(idExist);
        if (m_videoDev[m_videoDevNum]->getDeviceId() >= 0) {
            idExist = m_videoDev[m_videoDevNum++]->getDeviceId()+1;
        }
    }

    ctrllog::warn("show video: {}. Video device number: {}", m_showVideo, m_videoDevNum);
    m_videoThread.start();
}

VideoCtrl::~VideoCtrl()
{
    if (m_videoThread.getRunState()) {
        m_videoThread.stop();
    }

    for (int32_t id = 0; id < m_videoDevNum; id++) {
        delete m_videoDev[id];
    }
}

void VideoCtrl::videoThreadFun(void *ctxt)
{
    VideoCtrl *obj = static_cast<VideoCtrl *>(ctxt);
    if (obj->m_videoDev[0]->getDeviceId() < 0) {
        ctrllog::warn("no video capture found...");
        return;
    }

    Mat frame, gray, edge;
    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto& video = obj->m_videoDev[0]->getVideoCapture();
        video >> frame;
        if (frame.empty()) {
            //ctrllog::warn("device read video error...");
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
