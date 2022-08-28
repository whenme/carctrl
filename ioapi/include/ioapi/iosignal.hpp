// SPDX-License-Identifier: GPL-2.0

#ifndef __IO_SIGNAL_HPP__
#define __IO_SIGNAL_HPP__

#include <vector>
#include <functional>

#include <asio.hpp>

class IoSignal final
{
public:
    IoSignal(asio::io_service& io_service, std::initializer_list<int32_t>& signalNum,
             std::function<void(const asio::error_code &e, void *ctxt)> signalHandler,
             void *ctxt = nullptr);
    virtual ~IoSignal();

private:
    std::vector<int32_t> m_signalVect;
    asio::io_service&    m_ioService;
    asio::signal_set     m_sigset;
    std::function<void(const asio::error_code &e, void *ctxt)> m_signalHandler;
    void *m_usrContext;
};

#endif
