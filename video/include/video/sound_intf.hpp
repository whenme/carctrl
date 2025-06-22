// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <string>
#include <xapi/iothread.hpp>
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
    bool     m_state{false};
    std::vector<std::string> m_vectContent;
    static constexpr int32_t m_interval = 100;

    void showWelcome();
    static void soundThreadFun(void *ctxt);
};
