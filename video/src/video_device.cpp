// SPDX-License-Identifier: GPL-2.0

#include <xapi/easylog.hpp>
#include <video/video_device.hpp>
#include <opencv2/videoio/legacy/constants_c.h>

VideoDevice::VideoDevice(int32_t id)
{
    if (m_videoDev.open(id)) {
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
        ctrllog::info("frame width={} height={} fps={}", frame.cols, frame.rows, m_videoParam.cameraFps);
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

CameraParam& VideoDevice::getDeviceParam()
{
    return m_videoParam;
}

int32_t VideoDevice::setDeviceParam(CameraParam param)
{
    int32_t ret = m_videoDev.set(CAP_PROP_FRAME_WIDTH, param.cameraWidth);
    if (!ret)
        m_videoParam.cameraWidth = param.cameraWidth;
    else {
        ctrllog::warn("fail to set video width...");
        return ret;
    }

    ret = m_videoDev.set(CAP_PROP_FRAME_HEIGHT, param.cameraHeight);
    if (!ret)
        m_videoParam.cameraHeight = param.cameraHeight;
    else {
        ctrllog::warn("fail to set video height...");
        return ret;
    }

    ret = m_videoDev.set(CAP_PROP_FPS, param.cameraFps);
    if (!ret)
        m_videoParam.cameraFps = param.cameraFps;
    else
        ctrllog::warn("fail to set video fps...");

    return ret;
}
