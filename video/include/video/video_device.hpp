// SPDX-License-Identifier: GPL-2.0
#ifndef __VIDEO_DEVICE_HPP__
#define __VIDEO_DEVICE_HPP__

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
    VideoDevice();
    virtual ~VideoDevice();

    VideoCapture& getVideoCapture();
    bool          getDeviceState();
    int32_t       setDeviceParam(CameraParam& param);
    void          getDeviceParam(CameraParam& param);

private:
    VideoCapture m_videoDev;
    CameraParam  m_videoParam;
    bool         m_state {false};
};

#endif