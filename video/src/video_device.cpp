// SPDX-License-Identifier: GPL-2.0

#include <xapi/easylog.hpp>
#include <video/video_device.hpp>
#include <opencv2/videoio/legacy/constants_c.h>

VideoDevice::VideoDevice(int32_t idStart)
{
    for (int32_t id = idStart; id <= video_device_max; id++) {
        if (m_videoCap.open(id)) { //found video id
            ctrllog::info("video capture {} exist!", id);
            m_devId = id;
            break;
        }
    }

    if (m_devId >= 0) {
        //set default parameter
        m_videoCap.set(CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
        m_videoCap.set(CAP_PROP_FPS, 10);
        m_videoCap.set(CAP_PROP_FRAME_WIDTH, video_frame_width);
        m_videoCap.set(CAP_PROP_FRAME_HEIGHT, video_frame_height);

        Mat frame;
        m_videoCap >> frame;
        m_videoParam.cameraWidth = frame.cols;  //m_videoCap.get(CAP_PROP_FRAME_WIDTH);
        m_videoParam.cameraHeight = frame.rows; //m_videoCap.get(CAP_PROP_FRAME_HEIGHT);
        m_videoParam.cameraFps = m_videoCap.get(CAP_PROP_FPS);
        ctrllog::info("frame width={} height={} fps={}", frame.cols, frame.rows, m_videoParam.cameraFps);
    }
}

VideoDevice::~VideoDevice()
{
    if (m_devId >= 0) {
        m_videoCap.release();
    }
}

VideoCapture& VideoDevice::getVideoCapture()
{
    return m_videoCap;
}

int32_t VideoDevice::getDeviceParam(CameraParam &param)
{
    if (m_devId < 0) {
        return -1;
    } else {
        param = m_videoParam;
        return 0;
    }
}

int32_t VideoDevice::getDeviceId()
{
    return m_devId;
}

int32_t VideoDevice::setDeviceParam(CameraParam param)
{
    int32_t ret = m_videoCap.set(CAP_PROP_FRAME_WIDTH, param.cameraWidth);
    if (!ret) {
        m_videoParam.cameraWidth = param.cameraWidth;
    } else {
        ctrllog::warn("fail to set video width...");
        return ret;
    }

    ret = m_videoCap.set(CAP_PROP_FRAME_HEIGHT, param.cameraHeight);
    if (!ret) {
        m_videoParam.cameraHeight = param.cameraHeight;
    } else {
        ctrllog::warn("fail to set video height...");
        return ret;
    }

    ret = m_videoCap.set(CAP_PROP_FPS, param.cameraFps);
    if (!ret) {
        m_videoParam.cameraFps = param.cameraFps;
    } else {
        ctrllog::warn("fail to set video fps...");
    }

    return ret;
}
