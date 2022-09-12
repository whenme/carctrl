// SPDX-License-Identifier: GPL-2.0

#include <stdlib.h>
#include <iostream>
#include <ioapi/cmn_singleton.hpp>
#include <video/sound_intf.hpp>

SoundIntf::SoundIntf()
:m_ios(1),
 m_iow(m_ios),
 m_iosThread("sound thread", IoThread::ThreadPriorityNormal, threadFun, this),
 m_timer(m_ios, timerCallback, this),
 m_state(true)
{
    system("pulseaudio --start"); //start pulseaudio service
    m_iosThread.start();
}

SoundIntf::~SoundIntf()
{
    m_iosThread.stop();
    m_ios.stop();
}

int32_t SoundIntf::speak(std::string content)
{
    m_vectContent.push_back(content);
    m_timer.start(m_interval);
    return 0;
}

void SoundIntf::timerCallback(const asio::error_code &e, void *ctxt)
{
    SoundIntf* obj = static_cast<SoundIntf*>(ctxt);
    if (obj->m_vectContent.size()) {
        std::string content = *(obj->m_vectContent.begin());
        obj->m_vectContent.erase(obj->m_vectContent.begin());

        if (obj->getSoundState() == true) {
            std::string speakContent = "ekho " + content;
            system(speakContent.c_str());
        }

        if (obj->m_vectContent.size()) {
            obj->m_timer.start(obj->m_interval);
        }
    }
}

void SoundIntf::threadFun(void *ctxt)
{
    SoundIntf* obj = static_cast<SoundIntf*>(ctxt);
    obj->m_ios.run();
}

void SoundIntf::setSoundState(bool enable)
{
    m_state = enable;
}

bool SoundIntf::getSoundState()
{
    return m_state;
}