// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <opencv2/opencv.hpp>

using namespace cv;

struct CameraParam
{
    int32_t cameraWidth;
    int32_t cameraHeight;
    int32_t cameraFps;
};

class VideoDevice
{
public:
    VideoDevice(int32_t idStart = 0);
    virtual ~VideoDevice();

    VideoCapture& getVideoCapture();

    // when did >=0, video device exist
    int32_t getDeviceId();
    int32_t setDeviceParam(CameraParam param);
    int32_t getDeviceParam(CameraParam &param);

private:
    VideoCapture m_videoCap;
    CameraParam  m_videoParam;
    int32_t      m_devId {-1};

    static constexpr int32_t video_device_max   = 6;
    static constexpr int32_t video_frame_height = 720;
    static constexpr int32_t video_frame_width  = 1280;
};
