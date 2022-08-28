// SPDX-License-Identifier: GPL-2.0

#include <ioapi/iotimer.hpp>

IoTimer::IoTimer(asio::io_service& io_service,
                 std::function<void(const asio::error_code &e, void *ctxt)> timeoutHandler,
                 void *ctxt, bool isRepeated) :
    m_ioService(io_service),
    m_timer(io_service),
    m_timeoutHandler(std::move(timeoutHandler)),
    m_usrContext(ctxt),
    m_repeat(isRepeated)
{
}

IoTimer::~IoTimer()
{
    stop();
}

void IoTimer::start(uint64_t timeoutms)
{
    reset(timeoutms);
}

void IoTimer::stop()
{
    m_isRunning.store(false);
}

void IoTimer::reset(uint64_t timeoutms)
{
    m_isRunning.store(true);
    doSetExpired(timeoutms);
}

void IoTimer::doSetExpired(uint64_t timeoutms)
{
    if (!m_isRunning.load())
        return;

    m_timer.expires_from_now(std::chrono::milliseconds(timeoutms));
    m_timer.async_wait([this, timeoutms](const asio::error_code &e) {
        if (e.value() == asio::error::operation_aborted || !m_isRunning.load())
            return;

        m_timeoutHandler(e, m_usrContext);
        if (m_repeat)
            this->doSetExpired(timeoutms);
    });
}

