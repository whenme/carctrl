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
    int32_t sing();
    void    setSoundState(int32_t enable);

private:
    IoThread m_iosThread;
    bool     m_state;
    std::vector<std::string> m_vectContent;
    static constexpr int32_t m_interval = 100;

    void showWelcome();
    static void threadFun(void *ctxt);
};

#endif
