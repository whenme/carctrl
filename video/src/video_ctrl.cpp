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
    m_webJpeg.resize(video_dev_num);
    m_hasWebFrame.resize(video_dev_num, false);

    //check there is GUI backend or not
    cv::Mat img = cv::imread("lena.png");
    if (!img.empty()) {
        try {
            //cv::imshow("lena", img);
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
        m_videoDev[vid] = new VideoDevice(idExist);
        if (m_videoDev[vid]->getDeviceId() >= 0) {
            m_videoDevNum++;
            idExist = m_videoDev[vid]->getDeviceId() + 1;
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

    for (int32_t id = 0; id < video_dev_num; id++) {
        delete m_videoDev[id];
        m_videoDev[id] = nullptr;
    }
}

void VideoCtrl::videoThreadFun(void *ctxt)
{
    VideoCtrl *obj = static_cast<VideoCtrl *>(ctxt);
    const bool hasCam0 = obj->isCameraExist(0);
    const bool hasCam1 = obj->isCameraExist(1);
    if (!hasCam0 && !hasCam1) {
        ctrllog::warn("no video capture found...");
        return;
    }

    cv::Mat frame, gray, edge;
    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        cv::Mat frameL, frameR;
        bool hasFrameL = false;
        bool hasFrameR = false;

        if (hasCam0) {
            auto& videoL = obj->m_videoDev[0]->getVideoCapture();
            videoL >> frameL;
            hasFrameL = !frameL.empty();
            if (hasFrameL) {
                obj->updateWebFrame(0, frameL);
            }
        }

        if (hasCam1) {
            auto& videoR = obj->m_videoDev[1]->getVideoCapture();
            videoR >> frameR;
            hasFrameR = !frameR.empty();
            if (hasFrameR) {
                obj->updateWebFrame(1, frameR);
            }
        }

        if (obj->m_stereoMode && hasCam0 && hasCam1) {
            if (!hasFrameL || !hasFrameR) {
                continue;
            }
            obj->showImage("left", frameL);
            obj->showImage("right", frameR);

            cv::Mat rectL, rectR;
            obj->m_stereoVision.rectifyPair(frameL, frameR, rectL, rectR);

            cv::Mat disparity, depth;
            obj->m_stereoVision.computeDisparityFromRectified(rectL, rectR, disparity, depth);

            obj->m_stereoObjectMatch.tryDetectAndLog(rectL, rectR);

            cv::Mat colorDisp;
            obj->m_stereoVision.getDisparityColor(disparity, colorDisp);
            if (!colorDisp.empty()) {
                obj->showImage("disparity", colorDisp);
            }

            if (!depth.empty()) {
                float dist = obj->m_stereoVision.getDistance(depth, depth.cols / 2, depth.rows / 2);
                if (dist > 0) {
                    ctrllog::warn("stereo center distance: {:.2f} m", dist);
                }
            }

            const auto fps = obj->m_videoDev[0]->getVideoCapture().get(CAP_PROP_FPS);
            cv::waitKey(1000 / fps);
            continue;
        }

        if (hasFrameL) {
            frame = frameL;
        } else if (hasFrameR) {
            frame = frameR;
        } else {
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
        const int32_t fpsCam = hasFrameL ? 0 : 1;
        const auto fps = obj->m_videoDev[fpsCam]->getVideoCapture().get(CAP_PROP_FPS);
        cv::waitKey(1000 / fps);
    }
}

void VideoCtrl::showImage(std::string title, Mat& mat)
{
    if (m_showVideo) {
        //cv::imshow(title, mat);
    }
}

void VideoCtrl::updateWebFrame(int32_t camera, const cv::Mat& mat)
{
    if (mat.empty() || camera < 0 || camera >= static_cast<int32_t>(m_webJpeg.size())) {
        return;
    }

    std::vector<uint8_t> encoded;
    const std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
    if (!cv::imencode(".jpg", mat, encoded, params)) {
        return;
    }

    std::lock_guard lock(m_webFrameMutex);
    m_webJpeg[camera] = std::move(encoded);
    m_hasWebFrame[camera] = true;
}

bool VideoCtrl::getWebFrame(int32_t camera, std::vector<uint8_t>& out)
{
    if (camera < 0 || camera >= static_cast<int32_t>(m_webJpeg.size())) {
        return false;
    }

    std::lock_guard lock(m_webFrameMutex);
    if (!m_hasWebFrame[camera]) {
        return false;
    }

    out = m_webJpeg[camera];
    return true;
}

int32_t VideoCtrl::getWebCameraCount()
{
    return m_videoDevNum;
}

bool VideoCtrl::isCameraExist(int32_t camera)
{
    if (camera < 0 || camera >= video_dev_num || m_videoDev[camera] == nullptr) {
        return false;
    }

    return m_videoDev[camera]->getDeviceId() >= 0;
}
