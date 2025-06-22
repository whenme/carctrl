// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>

#include <xapi/cmn_singleton.hpp>
#include <video/sound_intf.hpp>

SoundIntf::SoundIntf():
  m_iosThread("sound thread", IoThread::ThreadPriorityNormal, soundThreadFun, this)
{
    system("pulseaudio --start"); //start pulseaudio service

    setSoundState(m_state);
}

SoundIntf::~SoundIntf()
{
    if (m_iosThread.getRunState())
        m_iosThread.stop();
}

int32_t SoundIntf::speak(std::string content)
{
    if (m_state)
        m_vectContent.push_back(content);

    return 0;
}

int32_t SoundIntf::sing()
{
    auto fun = []() {
        system("mplayer -quiet 长相思.mp3 -volume 70");
    };
    std::thread th(fun);
    th.detach();
    return 0;
}

void SoundIntf::soundThreadFun(void *ctxt)
{
    SoundIntf* obj = static_cast<SoundIntf*>(ctxt);

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(obj->m_interval));

        if (obj->m_vectContent.size()) {
            std::string content = *(obj->m_vectContent.begin());
            obj->m_vectContent.erase(obj->m_vectContent.begin());

            if (obj->m_state == true) {
                std::string speakContent = "ekho " + content;
                system(speakContent.c_str());
            }
        }
    }
}

void SoundIntf::setSoundState(int32_t enable)
{
    m_vectContent.clear();
    m_state = enable ? true : false;
    if (m_state) {
        if (!m_iosThread.getRunState()) {
            m_iosThread.start();
            m_iosThread.setCpuAffinity(2);
        }

        showWelcome();
    } else {
        m_vectContent.clear();
    }
}

void SoundIntf::showWelcome()
{
    speak("你好, 上海, 人工智能车");
    speak("artificial intelligence car demo");
}
