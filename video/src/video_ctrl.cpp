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
    cv::Mat img = cv::imread("lena.png");
    if (!img.empty()) {
        try {
            cv::imshow("lena", img);
            cv::destroyWindow("lena");
        }
        catch(...) {
            // No GUI backend...
            m_showVideo = false;
        }
    }

    //find video device id. some video device begin from /dev/video4 as OPI3B
    int32_t idExist = 0;
    for (int32_t vid = 0; vid < video_dev_num; vid++) {
        m_videoDev[m_videoDevNum] = new VideoDevice(idExist);
        if (m_videoDev[m_videoDevNum]->getDeviceId() >= 0) {
            idExist = m_videoDev[m_videoDevNum++]->getDeviceId()+1;
        }
    }

    ctrllog::warn("show video: {}. Video device number: {}", m_showVideo, m_videoDevNum);
    if (!m_videoDevNum) {
        m_showVideo = false;
    }

    // Try to enter stereo mode when two cameras are available
    if (m_videoDevNum >= 2) {
        if (m_stereoVision.loadCalibration(stereo_calib_file)) {
            CameraParam param;
            m_videoDev[0]->getDeviceParam(param);
            m_stereoVision.initRectifyMap(cv::Size(param.cameraWidth, param.cameraHeight));
            m_stereoMode = true;
            ctrllog::info("stereo mode enabled with {} cameras", m_videoDevNum);
        } else {
            ctrllog::warn("stereo calibration not found, falling back to mono mode");
        }
    }

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

    cv::Mat frame, gray, edge;
    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Stereo mode: dual-camera disparity/depth
        if (obj->m_stereoMode && obj->m_videoDevNum >= 2) {
            cv::Mat frameL, frameR;
            auto& videoL = obj->m_videoDev[0]->getVideoCapture();
            auto& videoR = obj->m_videoDev[1]->getVideoCapture();
            videoL >> frameL;
            videoR >> frameR;
            if (frameL.empty() || frameR.empty()) {
                continue;
            }
            obj->showImage("left", frameL);
            obj->showImage("right", frameR);

            cv::Mat disparity, depth;
            obj->m_stereoVision.computeDisparity(frameL, frameR, disparity, depth);

            cv::Mat colorDisp;
            obj->m_stereoVision.getDisparityColor(disparity, colorDisp);
            if (!colorDisp.empty()) {
                obj->showImage("disparity", colorDisp);
            }

            // Log center-point distance
            if (!depth.empty()) {
                float dist = obj->m_stereoVision.getDistance(depth, depth.cols / 2, depth.rows / 2);
                if (dist > 0) {
                    ctrllog::warn("stereo center distance: {:.2f} m", dist);
                }
            }

            cv::waitKey(1000 / videoL.get(CAP_PROP_FPS));
            continue;
        }

        // Mono mode: single-camera lane detection
        auto& video = obj->m_videoDev[0]->getVideoCapture();
        video >> frame;
        if (frame.empty()) { //device read video error
            continue;
        }
        obj->showImage("capture", frame);

        cv::cvtColor(frame, gray, COLOR_BGR2GRAY); //to gray
        cv::GaussianBlur(gray, gray, Size(5, 5), 0);  //gauss filter
        obj->showImage("gray", gray);

        //co_await lf::fork[GaussianBlur](gray, gray, Size(5, 5), 0);
        //co_await lf::just(GaussianBlur)(gray, gray, Size(5, 5), 0);  //gauss filter
        //lf::sync_wait(lf::lazy_pool{}, wrapperFun, GaussianBlur, gray, gray, Size(5, 5), 0, 0, BORDER_DEFAULT);
        //lf::wrapperTaskFun(GaussianBlur, gray, gray, Size(5, 5), 0.0, 0.0, BORDER_DEFAULT);
        //cmn::wrapperFunc(GaussianBlur, gray, gray, Size(5, 5), 0.0, 0.0, BORDER_DEFAULT);

        cv::Canny(gray, edge, 40, 100, 3, false); //edge detection
        //threshold(edge, edge, 170, 255, THRESH_BINARY);

        std::vector<Vec4i> lineP1;
        cv::HoughLinesP(edge, lineP1, 1, CV_PI/180, 90, 30, 40); //100/90
        for (size_t i = 0; i < lineP1.size(); i++) { //draw line
            cv::line(edge, Point(lineP1[i][0], lineP1[i][1]), Point(lineP1[i][2], lineP1[i][3]), Scalar(255), 3);
        }
        obj->showImage("edge", edge);
        cv::waitKey(1000 / video.get(CAP_PROP_FPS));
    }
}

void VideoCtrl::showImage(std::string title, Mat& mat)
{
    if (m_showVideo) {
        cv::imshow(title, mat);
    }
}
