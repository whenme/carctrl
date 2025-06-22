// SPDX-License-Identifier: GPL-2.0

#include <xapi/iotimer.hpp>

IoTimer::IoTimer(asio::io_context& context,
                 std::function<void(const asio::error_code &e, void *ctxt)> timeoutHandler,
                 void *ctxt, bool isRepeated) :
    m_context(context),
    m_timer(context),
    m_timeoutHandler(std::move(timeoutHandler)),
    m_usrContext(ctxt),
    m_repeat(isRepeated)
{
}

IoTimer::~IoTimer()
{
    stop();
    m_timer.cancel();
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

