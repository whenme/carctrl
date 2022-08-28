// SPDX-License-Identifier: GPL-2.0

#include <ioapi/iosignal.hpp>

IoSignal::IoSignal(asio::io_service& io_service, std::initializer_list<int32_t>& signalNum,
                   std::function<void(const asio::error_code &e, void *ctxt)> signalHandler,
                   void *ctxt):
    m_ioService(io_service),
    m_sigset(m_ioService),
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
