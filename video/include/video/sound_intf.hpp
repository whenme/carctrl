// SPDX-License-Identifier: GPL-2.0
#ifndef __SOUND_INTF_HPP__
#define __SOUND_INTF_HPP__

#include <string>
#include <ioapi/iotimer.hpp>
#include <ioapi/iothread.hpp>
#include "asio.hpp"

class SoundIntf
{
public:
    SoundIntf();
    virtual ~SoundIntf();

    int32_t speak(std::string content);
    void    setSoundState(int32_t enable);
    bool    getSoundState();

private:
    asio::io_service m_ios;
    asio::io_service::work m_iow;
    IoThread m_iosThread;
    IoTimer  m_timer;
    bool     m_state;
    std::vector<std::string> m_vectContent;
    static constexpr int32_t m_interval = 100;

    void showWelcome();
    static void timerCallback(const asio::error_code &e, void *ctxt);
    static void threadFun(void *ctxt);
};

#endif
