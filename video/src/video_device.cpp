// SPDX-License-Identifier: GPL-2.0

#include <video/video_device.hpp>

VideoDevice::VideoDevice()
{
    m_videoDev = VideoCapture(0);
    if (!m_videoDev.isOpened())
        m_videoDev = VideoCapture(1);

    if (!m_videoDev.isOpened()) {
        std::cout << "Cannot find video device..." << std::endl;
    } else {
        m_state = true;
        m_videoDev.set(CAP_PROP_FRAME_WIDTH, 1280);
        m_videoDev.set(CAP_PROP_FRAME_HEIGHT, 720);

        m_videoParam.cameraWidth = m_videoDev.get(CAP_PROP_FRAME_WIDTH);
        m_videoParam.cameraHeight = m_videoDev.get(CAP_PROP_FRAME_HEIGHT);
        m_videoParam.cameraFps = m_videoDev.get(CAP_PROP_FPS);
        std::cout << "video parameter: width " << m_videoParam.cameraWidth << " height "
                  << m_videoParam.cameraHeight << " Fps " << m_videoParam.cameraFps << std::endl;
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

void VideoDevice::getDeviceParam(CameraParam& param)
{
    param.cameraWidth = m_videoParam.cameraWidth;
    param.cameraHeight = m_videoParam.cameraHeight;
    param.cameraFps = m_videoParam.cameraFps;
}

int32_t VideoDevice::setDeviceParam(CameraParam& param)
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