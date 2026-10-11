// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace asio
{
class io_context;
}

class VideoCtrl
{
public:
    explicit VideoCtrl(asio::io_context& ioContext);
    virtual ~VideoCtrl();

    bool getWebFrame(int32_t camera, std::vector<uint8_t>& out);
    int32_t getWebCameraCount();
    bool isCameraExist(int32_t camera);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
