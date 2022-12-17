// SPDX-License-Identifier: GPL-2.0

#include <video/video_device.hpp>
#include <opencv2/videoio/legacy/constants_c.h>

VideoDevice::VideoDevice()
{
    for (int32_t i = 0; i < 2; i++) {
        bool ret = m_videoDev.open(i);
        if (ret) {
            std::cout << "video capture " << i << " exist!" <<std::endl;
            break;
        }
    }

    if (!m_videoDev.isOpened()) {
        std::cout << "Cannot find video device..." << std::endl;
    } else {
        m_state = true;

        //set default parameter
        m_videoDev.set(CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
        m_videoDev.set(CAP_PROP_FPS, 10);
        m_videoDev.set(CAP_PROP_FRAME_WIDTH, video_frame_width);
        m_videoDev.set(CAP_PROP_FRAME_HEIGHT, video_frame_height);

        Mat frame;
        m_videoDev >> frame;
        m_videoParam.cameraWidth = frame.cols;  //m_videoDev.get(CAP_PROP_FRAME_WIDTH);
        m_videoParam.cameraHeight = frame.rows; //m_videoDev.get(CAP_PROP_FRAME_HEIGHT);
        m_videoParam.cameraFps = m_videoDev.get(CAP_PROP_FPS);
        std::cout << "frame width=" << frame.cols << " height=" << frame.rows
                  << " fps=" << m_videoParam.cameraFps << std::endl;
    }
}

VideoDevice::~VideoDevice()
{
    m_videoDev.release();
}

VideoCapture& VideoDevice::getVideoCapture()
{
    return m_videoDev;
}

bool VideoDevice::getDeviceState()
{
    return m_state;
}

CameraParam& VideoDevice::getDeviceParam()
{
    return m_videoParam;
}

int32_t VideoDevice::setDeviceParam(CameraParam param)
{
    if (!m_state) {
        std::cout << "video device error..." << std::endl;
        return -1;
    }

    int32_t ret = m_videoDev.set(CAP_PROP_FRAME_WIDTH, param.cameraWidth);
    if (!ret)
        m_videoParam.cameraWidth = param.cameraWidth;
    else {
        std::cout << "fail to set video width..." << std::endl;
        return ret;
    }

    ret = m_videoDev.set(CAP_PROP_FRAME_HEIGHT, param.cameraHeight);
    if (!ret)
        m_videoParam.cameraHeight = param.cameraHeight;
    else {
        std::cout << "fail to set video height..." << std::endl;
        return ret;
    }

    ret = m_videoDev.set(CAP_PROP_FPS, param.cameraFps);
    if (!ret)
        m_videoParam.cameraFps = param.cameraFps;
    else
        std::cout << "fail to set video fps..." << std::endl;

    return ret;
}