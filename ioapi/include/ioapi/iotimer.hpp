// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <functional>
#include <atomic>
#include <asio.hpp>

class IoTimer final
{
public:
    IoTimer(asio::io_service& io_service,
            std::function<void(const asio::error_code &e, void *ctxt)> timeoutHandler,
            void *ctxt = nullptr, bool isRepeated = false);
    virtual ~IoTimer();

    void start(uint64_t timeoutms);
    void reset(uint64_t timeoutms);
    void stop();

private:
    void doSetExpired(uint64_t timeoutms);

    asio::io_service&  m_ioService;
    asio::steady_timer m_timer;
    bool               m_repeat;
    std::atomic<bool>  m_isRunning{false};

    // The handler that will be triggered once the time's up.
    std::function<void(const asio::error_code &e, void *ctxt)> m_timeoutHandler;
    void *m_usrContext;
};
