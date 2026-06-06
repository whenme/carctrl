// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <memory>
#include <string>

#include <asio.hpp>
#include <cli_car.hpp>
#include <video/video_ctrl.hpp>

namespace cli
{

class CarWebServer
{
public:
    CarWebServer(asio::io_context& context, CliCar& cliCar, VideoCtrl& videoCtrl, uint16_t port);
    ~CarWebServer();

private:
    void accept();
    void handleConnection(std::shared_ptr<asio::ip::tcp::socket> socket);
    void handleCameraStream(std::shared_ptr<asio::ip::tcp::socket> socket, int32_t cameraId);
    void sendCameraFrame(std::shared_ptr<asio::ip::tcp::socket> socket, int32_t cameraId);

    std::string buildStatusJson();
    std::string buildCameraInfoJson();
    std::string handleRequest(const std::string& request);
    std::string httpResponse(int code, const std::string& status,
                             const std::string& contentType, const std::string& body);
    static std::string getRequestPath(const std::string& request);
    static int32_t getCameraStreamId(const std::string& path);
    static std::string urlDecode(const std::string& value);
    static std::string getQueryParam(const std::string& query, const std::string& key);

    asio::io_context&       m_context;
    CliCar&                 m_cliCar;
    VideoCtrl&              m_videoCtrl;
    asio::ip::tcp::acceptor m_acceptor;
};

}  // namespace cli
