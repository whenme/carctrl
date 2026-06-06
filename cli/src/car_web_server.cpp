// SPDX-License-Identifier: GPL-2.0

#include <car_web_server.hpp>

#include <cctype>
#include <array>
#include <chrono>
#include <functional>
#include <sstream>

#include <fmt/format.h>
#include <xapi/easylog.hpp>
#include <rpc_service.hpp>

using namespace async_simple::coro;

namespace cli
{

namespace
{

constexpr const char* k_indexHtml = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Car Control</title>
<style>
  :root { color-scheme: light dark; font-family: system-ui, sans-serif; }
  body { margin: 0; padding: 1.5rem; background: #0f172a; color: #e2e8f0; }
  h1 { margin: 0 0 1rem; font-size: 1.5rem; }
  .layout {
    display: grid;
    grid-template-columns: minmax(280px, 420px) 1fr;
    gap: 1rem;
    align-items: start;
  }
  .left-panel { display: flex; flex-direction: column; gap: 1rem; }
  .card { background: #1e293b; border-radius: 12px; padding: 1rem; box-shadow: 0 8px 24px rgba(0,0,0,.25); }
  .camera-panel img {
    width: 100%;
    max-height: calc(100vh - 8rem);
    object-fit: contain;
    background: #000;
    border-radius: 8px;
    display: block;
  }
  .camera-grid { display: grid; gap: .75rem; grid-template-columns: 1fr; }
  .camera-feed span { color: #94a3b8; font-size: .85rem; }
  .camera-feed { display: flex; flex-direction: column; gap: .35rem; }
  .camera-placeholder {
    width: 100%;
    min-height: 240px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #000;
    border-radius: 8px;
    color: #64748b;
    font-size: .95rem;
  }
  .camera-placeholder.hidden, #camera.hidden { display: none; }
  .layout.no-camera { grid-template-columns: 1fr; max-width: 420px; }
  .camera-panel.hidden, .camera-control.hidden { display: none; }
  @media (max-width: 900px) {
    .layout { grid-template-columns: 1fr; }
  }
  .card h2 { margin: 0 0 .75rem; font-size: 1rem; color: #94a3b8; }
  table { width: 100%; border-collapse: collapse; font-size: .95rem; }
  th, td { text-align: left; padding: .4rem .2rem; border-bottom: 1px solid #334155; }
  .controls { display: grid; gap: .5rem; }
  button, input[type=number] {
    border: 0; border-radius: 8px; padding: .65rem .8rem; font-size: .95rem;
  }
  button { background: #2563eb; color: #fff; cursor: pointer; }
  button.secondary { background: #475569; }
  button.danger { background: #dc2626; }
  button:active { transform: translateY(1px); }
  .row { display: flex; gap: .5rem; flex-wrap: wrap; align-items: center; }
  .dpad { display: grid; grid-template-columns: repeat(3, 1fr); gap: .5rem; max-width: 220px; }
  .dpad .spacer { visibility: hidden; }
  #statusText { color: #94a3b8; font-size: .9rem; min-height: 1.2rem; }
</style>
</head>
<body>
<h1>Car Control</h1>
<div class="layout">
  <div class="left-panel">
    <section class="card">
      <h2>Drive</h2>
      <div class="controls">
        <div class="row">
          <label>Speed level</label>
          <input id="speedLevel" type="number" min="1" max="9" value="1" style="width:4rem">
          <button onclick="setSpeed()">Apply</button>
        </div>
        <div class="row">
          <label>Steps</label>
          <input id="stepCount" type="number" value="10" style="width:5rem">
        </div>
        <div class="dpad">
          <div class="spacer"></div>
          <button onclick="moveCar(0, 1)">Forward</button>
          <div class="spacer"></div>
          <button onclick="moveCar(1, 1)">Left</button>
          <button class="danger" onclick="stopCar()">Stop</button>
          <button onclick="moveCar(1, -1)">Right</button>
          <div class="spacer"></div>
          <button onclick="moveCar(0, -1)">Back</button>
          <div class="spacer"></div>
        </div>
        <div class="row">
          <label>Steer</label>
          <button class="secondary" onclick="steer(-1)">Left</button>
          <button class="secondary" onclick="steer(0)">Center</button>
          <button class="secondary" onclick="steer(1)">Right</button>
        </div>
        <div class="row camera-control" id="cameraControlRow">
          <label>Camera</label>
          <button id="cameraToggle" class="secondary" onclick="toggleCamera()">Hide Camera</button>
        </div>
      </div>
    </section>
    <section class="card">
      <h2>Status</h2>
      <div id="summary">Loading...</div>
      <table>
        <thead><tr><th>Motor</th><th>Speed</th><th>PWM</th><th>Ctrl Step</th><th>Actual Step</th></tr></thead>
        <tbody id="motors"></tbody>
      </table>
    </section>
  </div>
  <section class="card camera-panel" id="cameraPanel">
    <h2>Camera Feed</h2>
    <div id="cameraFeeds" class="camera-grid single"></div>
    <div id="cameraPlaceholder" class="camera-placeholder hidden">Camera hidden</div>
  </section>
</div>
<p id="statusText"></p>
<script>
async function api(path, method='GET') {
  const res = await fetch(path, { method });
  return res.json();
}
function setStatus(msg) { document.getElementById('statusText').textContent = msg || ''; }
async function refresh() {
  try {
    const data = await api('/api/status');
    document.getElementById('summary').textContent =
      `Speed level ${data.speedLevel}, motors ${data.motorNum}, mode ${data.ctrlMode}`;
    const tbody = document.getElementById('motors');
    tbody.innerHTML = '';
    for (const motor of data.motors) {
      const row = document.createElement('tr');
      row.innerHTML = `<td>${motor.id}</td><td>${motor.speed}</td><td>${motor.pwm}</td><td>${motor.ctrlStep}</td><td>${motor.actualStep}</td>`;
      tbody.appendChild(row);
    }
  } catch (err) {
    setStatus('Failed to load status');
  }
}
async function setSpeed() {
  const level = document.getElementById('speedLevel').value;
  await api(`/api/speed?level=${level}`, 'POST');
  setStatus(`Speed level set to ${level}`);
  refresh();
}
async function moveCar(direction, sign) {
  const steps = document.getElementById('stepCount').value * sign;
  await api(`/api/step?direction=${direction}&steps=${steps}`, 'POST');
  setStatus(`Move direction=${direction}, steps=${steps}`);
  refresh();
}
async function stopCar() {
  await api('/api/stop', 'POST');
  setStatus('Motors stopped');
  refresh();
}
async function steer(dir) {
  await api(`/api/steer?dir=${dir}&time=500`, 'POST');
  setStatus(`Steer ${dir}`);
}
let cameraOn = true;
let activeCameras = [];
function cameraStreamUrl(index) {
  return index === 0 ? `/api/camera?t=${Date.now()}` : `/api/camera/${index}?t=${Date.now()}`;
}
async function initCameras() {
  const panel = document.getElementById('cameraPanel');
  const controlRow = document.getElementById('cameraControlRow');
  const layout = document.querySelector('.layout');
  const container = document.getElementById('cameraFeeds');
  try {
    const info = await api('/api/camera/info');
    activeCameras = (info.cameras || []).filter(c => c.exists).map(c => c.id);
  } catch (err) {
    activeCameras = [];
  }
  if (activeCameras.length <= 0) {
    panel.classList.add('hidden');
    controlRow.classList.add('hidden');
    layout.classList.add('no-camera');
    container.innerHTML = '';
    return;
  }
  panel.classList.remove('hidden');
  controlRow.classList.remove('hidden');
  layout.classList.remove('no-camera');
  container.className = 'camera-grid';
  container.innerHTML = '';
  for (const id of activeCameras) {
    const feed = document.createElement('div');
    feed.className = 'camera-feed';
    feed.dataset.cameraId = id;
    const label = document.createElement('span');
    label.textContent = activeCameras.length >= 2 ? `Camera ${id + 1}` : 'Camera';
    const img = document.createElement('img');
    img.id = `camera${id}`;
    img.alt = `Camera ${id + 1} feed`;
    if (cameraOn) {
      img.src = cameraStreamUrl(id);
    }
    feed.appendChild(label);
    feed.appendChild(img);
    container.appendChild(feed);
  }
}
function toggleCamera() {
  if (activeCameras.length <= 0) {
    return;
  }
  const feeds = document.querySelectorAll('#cameraFeeds .camera-feed');
  const placeholder = document.getElementById('cameraPlaceholder');
  const button = document.getElementById('cameraToggle');
  cameraOn = !cameraOn;
  if (cameraOn) {
    feeds.forEach(feed => {
      const img = feed.querySelector('img');
      const id = Number(feed.dataset.cameraId);
      img.src = cameraStreamUrl(id);
      img.classList.remove('hidden');
    });
    placeholder.classList.add('hidden');
    button.textContent = 'Hide Camera';
  } else {
    feeds.forEach(feed => {
      const img = feed.querySelector('img');
      img.src = '';
      img.classList.add('hidden');
    });
    placeholder.classList.remove('hidden');
    button.textContent = 'Show Camera';
  }
}
initCameras();
refresh();
setInterval(refresh, 1000);
</script>
</body>
</html>)html";

}  // namespace

CarWebServer::CarWebServer(asio::io_context& context, CliCar& cliCar, VideoCtrl& videoCtrl, uint16_t port) :
    m_context{context},
    m_cliCar{cliCar},
    m_videoCtrl{videoCtrl},
    m_acceptor{context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)}
{
    ctrllog::info("car web server listening on port {}", port);
    accept();
}

CarWebServer::~CarWebServer()
{
    asio::error_code ec;
    m_acceptor.close(ec);
}

void CarWebServer::accept()
{
    auto socket = std::make_shared<asio::ip::tcp::socket>(m_context);
    m_acceptor.async_accept(*socket, [this, socket](const asio::error_code& ec) {
        if (!ec) {
            handleConnection(socket);
        }
        accept();
    });
}

void CarWebServer::handleConnection(std::shared_ptr<asio::ip::tcp::socket> socket)
{
    auto request = std::make_shared<std::string>();
    auto buffer = std::make_shared<std::array<char, 4096>>();

    auto readMore = std::make_shared<std::function<void()>>();
    *readMore = [this, socket, request, buffer, readMore]() {
        socket->async_read_some(asio::buffer(*buffer),
            [this, socket, request, buffer, readMore](const asio::error_code& ec, std::size_t length) {
                if (ec) {
                    return;
                }

                request->append(buffer->data(), length);
                if (request->find("\r\n\r\n") == std::string::npos) {
                    (*readMore)();
                    return;
                }

                const auto path = getRequestPath(*request);
                if (path == "/api/camera" || path == "/api/camera/0" || path == "/api/camera/1") {
                    handleCameraStream(socket, getCameraStreamId(path));
                    return;
                }

                const std::string response = handleRequest(*request);
                asio::async_write(*socket, asio::buffer(response),
                    [socket](const asio::error_code& writeEc, std::size_t) {
                        asio::error_code closeEc;
                        socket->shutdown(asio::ip::tcp::socket::shutdown_both, closeEc);
                        socket->close(closeEc);
                        (void)writeEc;
                    });
            });
    };

    (*readMore)();
}

void CarWebServer::handleCameraStream(std::shared_ptr<asio::ip::tcp::socket> socket, int32_t cameraId)
{
    if (!m_videoCtrl.isCameraExist(cameraId)) {
        const std::string response = httpResponse(404, "Not Found", "text/plain", "camera not found");
        asio::async_write(*socket, asio::buffer(response),
            [socket](const asio::error_code&, std::size_t) {
                asio::error_code ec;
                socket->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                socket->close(ec);
            });
        return;
    }

    static const std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    asio::async_write(*socket, asio::buffer(header),
        [this, socket, cameraId](const asio::error_code& ec, std::size_t) {
            if (!ec) {
                sendCameraFrame(socket, cameraId);
            }
        });
}

void CarWebServer::sendCameraFrame(std::shared_ptr<asio::ip::tcp::socket> socket, int32_t cameraId)
{
    auto jpeg = std::make_shared<std::vector<uint8_t>>();
    if (!m_videoCtrl.getWebFrame(cameraId, *jpeg) || jpeg->empty()) {
        auto timer = std::make_shared<asio::steady_timer>(m_context, std::chrono::milliseconds(100));
        timer->async_wait([this, socket, cameraId, timer](const asio::error_code& ec) {
            if (!ec) {
                sendCameraFrame(socket, cameraId);
            }
        });
        return;
    }

    auto partHeader = std::make_shared<std::string>(fmt::format(
        "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: {}\r\n\r\n", jpeg->size()));

    asio::async_write(*socket, asio::buffer(*partHeader),
        [this, socket, cameraId, partHeader, jpeg](const asio::error_code& ec, std::size_t) {
            if (ec) {
                return;
            }

            asio::async_write(*socket, asio::buffer(*jpeg),
                [this, socket, cameraId](const asio::error_code& writeEc, std::size_t) {
                    if (writeEc) {
                        return;
                    }

                    auto timer = std::make_shared<asio::steady_timer>(m_context, std::chrono::milliseconds(100));
                    timer->async_wait([this, socket, cameraId, timer](const asio::error_code& timerEc) {
                        if (!timerEc) {
                            sendCameraFrame(socket, cameraId);
                        }
                    });
                });
        });
}

int32_t CarWebServer::getCameraStreamId(const std::string& path)
{
    if (path == "/api/camera/1") {
        return 1;
    }
    return 0;
}

std::string CarWebServer::buildCameraInfoJson()
{
    std::ostringstream body;
    body << R"({"cameras":[)";
    for (int32_t i = 0; i < 2; ++i) {
        if (i > 0) {
            body << ',';
        }
        body << fmt::format(R"({{"id":{},"exists":{}}})", i,
                            m_videoCtrl.isCameraExist(i) ? "true" : "false");
    }
    body << "]}";
    return body.str();
}

std::string CarWebServer::getRequestPath(const std::string& request)
{
    const auto lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        return {};
    }

    const std::string requestLine = request.substr(0, lineEnd);
    std::istringstream stream(requestLine);
    std::string method;
    std::string target;
    std::string httpVersion;
    stream >> method >> target >> httpVersion;

    const auto queryPos = target.find('?');
    if (queryPos != std::string::npos) {
        return target.substr(0, queryPos);
    }
    return target;
}

std::string CarWebServer::httpResponse(int code, const std::string& status,
                                       const std::string& contentType, const std::string& body)
{
    return fmt::format(
        "HTTP/1.1 {} {}\r\n"
        "Content-Type: {}\r\n"
        "Content-Length: {}\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n{}",
        code, status, contentType, body.size(), body);
}

std::string CarWebServer::urlDecode(const std::string& value)
{
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const int hi = std::isxdigit(static_cast<unsigned char>(value[i + 1]))
                               ? std::tolower(static_cast<unsigned char>(value[i + 1]))
                               : -1;
            const int lo = std::isxdigit(static_cast<unsigned char>(value[i + 2]))
                               ? std::tolower(static_cast<unsigned char>(value[i + 2]))
                               : -1;
            if (hi >= 0 && lo >= 0) {
                decoded.push_back(static_cast<char>((hi <= 9 ? hi - '0' : hi - 'a' + 10) << 4 |
                                                    (lo <= 9 ? lo - '0' : lo - 'a' + 10)));
                i += 2;
                continue;
            }
        } else if (value[i] == '+') {
            decoded.push_back(' ');
            continue;
        }
        decoded.push_back(value[i]);
    }
    return decoded;
}

std::string CarWebServer::getQueryParam(const std::string& query, const std::string& key)
{
    const std::string prefix = key + "=";
    std::size_t start = 0;
    while (start < query.size()) {
        const auto end = query.find('&', start);
        const auto token = query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (token.rfind(prefix, 0) == 0) {
            return urlDecode(token.substr(prefix.size()));
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return {};
}

std::string CarWebServer::buildStatusJson()
{
    auto& client = m_cliCar.getClient();
    const int32_t motorNum = rpc_call_int_param<getMotorNum>(client);
    const int32_t speedLevel = rpc_call_int_param<getMotorSpeedLevel>(client);
    const int32_t ctrlMode = rpc_call_int_param<getCtrlMode>(client);

    std::ostringstream body;
    body << fmt::format(R"({{"speedLevel":{},"motorNum":{},"ctrlMode":{},"motors":[)",
                        speedLevel, motorNum, ctrlMode);

    for (int32_t ii = 1; ii <= motorNum; ++ii) {
        if (ii > 1) {
            body << ',';
        }
        body << fmt::format(
            R"({{"id":{},"speed":{},"pwm":{},"ctrlStep":{},"actualStep":{}}})",
            ii,
            rpc_call_int_param<getActualSpeed>(client, ii),
            rpc_call_int_param<getMotorPwm>(client, ii),
            rpc_call_int_param<getCtrlSteps>(client, ii),
            rpc_call_int_param<getActualSteps>(client, ii));
    }

    body << "]}";
    return body.str();
}

std::string CarWebServer::handleRequest(const std::string& request)
{
    const auto lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        return httpResponse(400, "Bad Request", "text/plain", "bad request");
    }

    const std::string requestLine = request.substr(0, lineEnd);
    std::istringstream stream(requestLine);
    std::string method;
    std::string target;
    std::string httpVersion;
    stream >> method >> target >> httpVersion;

    std::string path = target;
    std::string query;
    const auto queryPos = target.find('?');
    if (queryPos != std::string::npos) {
        path = target.substr(0, queryPos);
        query = target.substr(queryPos + 1);
    }

    if (method == "GET" && (path == "/" || path == "/index.html")) {
        return httpResponse(200, "OK", "text/html; charset=utf-8", k_indexHtml);
    }

    if (method == "GET" && path == "/api/status") {
        return httpResponse(200, "OK", "application/json", buildStatusJson());
    }

    if (method == "GET" && path == "/api/camera/info") {
        return httpResponse(200, "OK", "application/json", buildCameraInfoJson());
    }

    if (method == "POST" && path == "/api/stop") {
        rpc_call_void_param<setAllMotorState>(m_cliCar.getClient(), 0);
        return httpResponse(200, "OK", "application/json", R"({"ok":true})");
    }

    if (method == "POST" && path == "/api/speed") {
        const auto levelStr = getQueryParam(query, "level");
        if (levelStr.empty()) {
            return httpResponse(400, "Bad Request", "application/json", R"({"ok":false,"error":"missing level"})");
        }
        const int32_t level = std::stoi(levelStr);
        rpc_call_int_param<setMotorSpeedLevel>(m_cliCar.getClient(), level);
        return httpResponse(200, "OK", "application/json", R"({"ok":true})");
    }

    if (method == "POST" && path == "/api/step") {
        const auto directionStr = getQueryParam(query, "direction");
        const auto stepsStr = getQueryParam(query, "steps");
        if (directionStr.empty() || stepsStr.empty()) {
            return httpResponse(400, "Bad Request", "application/json",
                                R"({"ok":false,"error":"missing direction or steps"})");
        }

        const int32_t direction = std::stoi(directionStr);
        const int32_t steps = std::stoi(stepsStr);
        const int32_t motorNum = rpc_call_int_param<getMotorNum>(m_cliCar.getClient());

        if (direction == 0) {
            rpc_call_int_param<setCarSteps>(m_cliCar.getClient(), CarDirection::dirUp, steps);
        } else if (direction == 1) {
            rpc_call_int_param<setCarSteps>(m_cliCar.getClient(), CarDirection::dirLeft, steps);
        } else if (direction == 2) {
            if (motorNum < 4) {
                return httpResponse(400, "Bad Request", "application/json",
                                    R"({"ok":false,"error":"rotation requires 4 motors"})");
            }
            rpc_call_int_param<setCarSteps>(m_cliCar.getClient(), CarDirection::dirRotation, steps);
        } else {
            return httpResponse(400, "Bad Request", "application/json", R"({"ok":false,"error":"invalid direction"})");
        }

        return httpResponse(200, "OK", "application/json", R"({"ok":true})");
    }

    if (method == "POST" && path == "/api/steer") {
        const auto dirStr = getQueryParam(query, "dir");
        const auto timeStr = getQueryParam(query, "time");
        if (dirStr.empty()) {
            return httpResponse(400, "Bad Request", "application/json", R"({"ok":false,"error":"missing dir"})");
        }

        const int32_t dir = std::stoi(dirStr);
        const uint32_t time = timeStr.empty() ? 0U : static_cast<uint32_t>(std::stoul(timeStr));
        rpc_call_int_param<setSteerTurn>(m_cliCar.getClient(), dir, time);
        return httpResponse(200, "OK", "application/json", R"({"ok":true})");
    }

    return httpResponse(404, "Not Found", "text/plain", "not found");
}

}  // namespace cli
