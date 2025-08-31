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
    VideoDevice(int32_t id);
    virtual ~VideoDevice();

    VideoCapture& getVideoCapture();
    bool          getDeviceState();
    int32_t       setDeviceParam(CameraParam param);
    CameraParam&  getDeviceParam();

private:
    VideoCapture m_videoDev;
    CameraParam  m_videoParam;
    int32_t      m_devId;
    bool         m_state {false};

    static constexpr int32_t video_frame_height = 720;
    static constexpr int32_t video_frame_width  = 1280;
};
