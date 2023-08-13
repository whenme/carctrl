// SPDX-License-Identifier: GPL-2.0

#include <ioapi/iosignal.hpp>

IoSignal::IoSignal(asio::io_context& ioContext, std::initializer_list<int32_t>& signalNum,
                   std::function<void(const asio::error_code &e, void *ctxt)> signalHandler,
                   void *ctxt):
    m_context(ioContext),
    m_sigset(m_context),
    m_signalHandler(signalHandler),
    m_usrContext(ctxt)
{
    for (auto& item : signalNum) {
        m_signalVect.push_back(item);
        m_sigset.add(item);
    }

    m_sigset.async_wait([&](const asio::error_code &e, int32_t signal) {
        for (auto& item : m_signalVect) {
            if (signal == item) {
                m_signalHandler(e, m_usrContext);
            }
        }
    });
}

IoSignal::~IoSignal()
{
    for (auto& item : m_signalVect) {
        m_sigset.remove(item);
    }

    m_signalVect.clear();
    m_sigset.clear();
}

bool IoSignal::addSignal(int32_t signalNum)
{
    m_sigset.add(signalNum);
    return true;
}

bool IoSignal::removeSignal(int32_t signalNum)
{
    m_sigset.remove(signalNum);
    return true;
}
