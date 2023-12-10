// SPDX-License-Identifier: GPL-2.0
#include <linux/input.h>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <video/sound_intf.hpp>
#include <cli/remote_key.hpp>
#include <cli/cli_car.hpp>
#include <rpc_service/rpc_service.hpp>

RemoteKey::RemoteKey(asio::io_context& context) :
    m_context(context),
    m_timer(context, timerCallback, this, true)
{
    initIrKey();
    m_timer.start(k_checkTime);
}

RemoteKey::~RemoteKey()
{
    m_timer.stop();
    if (m_keyfd > 0)
        close(m_keyfd);
}

int32_t RemoteKey::initIrKey()
{
    char buf[128] {0};
    std::string fileName;

    //enable lirc
    system("echo '+rc-5 +nec +rc-6 +jvc +sony +rc-5-sz +sanyo +sharp +mce_kbd +xmp' > /sys/class/rc/rc0/protocols");

    //ir key file is /dev/input/event1 or /dev/input/event2. find it with shell
    FILE *ptr = popen("ls -l /dev/input/by-path |grep ir-event |grep -o 'event[0-5]'", "r");
    if (ptr != NULL) {
        if (fgets(buf, 128, ptr) != NULL) {
            fileName = std::string("/dev/input/") + buf;
            fileName.pop_back();   //remove last return
        }

        if (!fileName.empty()) {
            m_keyfd = open(fileName.c_str(), O_RDONLY | O_NONBLOCK);
            if (m_keyfd < 0)
                ctrllog::warn("fail to open {}", fileName);
        }
        pclose(ptr);
        ptr = NULL;
        return 0;
    }

    return -1;
}

void RemoteKey::timerCallback(const asio::error_code &e, void *ctxt)
{
    RemoteKey *pKey = static_cast<RemoteKey *>(ctxt);
    struct input_event ev[32];

    if (pKey->m_keyfd <= 0)  //key file error
        return;

    //read input_event
    int32_t evsize = read(pKey->m_keyfd, ev, sizeof(ev));
    if (evsize < (int32_t)sizeof(struct input_event))
        return;

    for (int32_t ii = 0; ii < evsize/sizeof(struct input_event); ii++) {
        if (4 == ev[ii].code) { //only handle key pressed
            if (ev[ii].value != 0) {
                pKey->m_keyList.push_back(ev[ii].value);
                break;
            }
        }
    }

    pKey->handleKeyPress();
}

int32_t RemoteKey::getKeyEvent(int32_t *event)
{
    std::list<int32_t> ::iterator it;
    if (!m_keyList.empty()) {
        it = m_keyList.begin();
        *event = *it;
        m_keyList.erase(it);
        return 0;
    } else {
        return -1;
    }
}

void RemoteKey::handleKeyPress()
{
    static int32_t input = 0;
    static int32_t oldKey = 0;
    static int32_t motorNum = 0;
    static bool    soundEnabled = false;

    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();
    auto& client = cmn::getSingletonInstance<cli::CliCar>().getClient();

    static IoTimer timerDirKey(m_context, [&](const asio::error_code &e, void *ctxt) {
        rpc_call_void_param<setAllMotorState>(client, 0);
    }, this, false);

    if (!motorNum) {
        motorNum = rpc_call_int_param<getMotorNum>(client);
    }

    int32_t keyEvent = 0;
    int32_t ret = getKeyEvent(&keyEvent);
    if (ret || (keyEvent < 0) || (keyEvent > 0xFF)) {
        //no key pressed or invalid key
        return;
    }

    bool repeatKey = false;
    if (keyEvent == oldKey) { // ignore repeated key, only handle arrow key
        if ((keyEvent != RC_KEY_UP) && (keyEvent != RC_KEY_DOWN)
            && (keyEvent != RC_KEY_LEFT) && (keyEvent != RC_KEY_RIGHT))
        {
            return;
        }
        repeatKey = true;
    }

    oldKey = keyEvent;
    ctrllog::info(" key {:x} is pressed. input {}", keyEvent, input);

    switch(keyEvent) {
    case RC_KEY_0:
        input = input*10;
        break;
    case RC_KEY_1:
        input = input*10 + 1;
        break;
    case RC_KEY_2:
        input = input*10 + 2;
        break;
    case RC_KEY_3:
        input = input*10 + 3;
        break;
    case RC_KEY_4:
        input = input*10 + 4;
        break;
    case RC_KEY_5:
        input = input*10 + 5;
        break;
    case RC_KEY_6:
        input = input*10 + 6;
        break;
    case RC_KEY_7:
        input = input*10 + 7;
        break;
    case RC_KEY_8:
        input = input*10 + 8;
        break;
    case RC_KEY_9:
        input = input*10 + 9;
        break;

    case RC_KEY_UP:
        if (input == 0) {
            rpc_call_int_param<setCarMoving>(client, CarDirection::dirUp);
            timerDirKey.start(k_stopTime);
            if (!repeatKey)
                soundIntf.speak("前进");
        } else {
            rpc_call_void_param<setCarSteps>(client, CarDirection::dirUp, input);
            soundIntf.speak(fmt::format("前进{}步", input));
            input = 0;
        }
        break;
    case RC_KEY_DOWN:
        if (input == 0) {
            rpc_call_int_param<setCarMoving>(client, CarDirection::dirDown);
            timerDirKey.start(k_stopTime);
            if (!repeatKey)
                soundIntf.speak("后退");
        } else {
            rpc_call_int_param<setCarSteps>(client, CarDirection::dirUp, -input);
            soundIntf.speak(fmt::format("后退{}步", input));
            input = 0;
        }
        break;
    case RC_KEY_LEFT:
        if (input == 0) {
            rpc_call_int_param<setCarMoving>(client, CarDirection::dirLeft);
            timerDirKey.start(k_stopTime);
            if (!repeatKey)
                soundIntf.speak("向左移动");
        } else {
            rpc_call_int_param<setCarSteps>(client, CarDirection::dirLeft, input);
            if (motorNum < 4) {
                soundIntf.speak(fmt::format("左轮前进{}步", input));
            } else {
                soundIntf.speak(fmt::format("左移{}步", input));
            }
            input = 0;
        }
        break;
    case RC_KEY_RIGHT:
        if (input == 0) {
            rpc_call_int_param<setCarMoving>(client, CarDirection::dirRight);
            timerDirKey.start(k_stopTime);
            if (!repeatKey)
                soundIntf.speak("向右移动");
        } else {
            rpc_call_int_param<setCarSteps>(client, CarDirection::dirLeft, -input);
            if (motorNum < 4) {
                soundIntf.speak(fmt::format("右轮前进{}步", input));
            } else {
                soundIntf.speak(fmt::format("右移{}步", input));
            }
            input = 0;
        }
        break;
    case RC_KEY_OK: // speed level
        if ((input < 1) || (input > 9)) {
            soundIntf.speak(fmt::format("速度等级{}不支持", input));
        } else {
            rpc_call_int_param<setMotorSpeedLevel>(client, input);
            ctrllog::info("set motor speed level {}", input);
            soundIntf.speak(fmt::format("设置速度等级{}", input));
        }
        input = 0;
        break;
    case RC_KEY_STAR: //sound on/off
        rpc_call_void_param<setAllMotorState>(client, 0);
        soundEnabled = soundEnabled? false:true;
        soundIntf.setSoundState(soundEnabled);
        input = 0;
        break;
    case RC_KEY_POUND: //rotation
        if (motorNum < 4) {
            soundIntf.speak("两轮车不支持");
        } else if (input == 0) {
            rpc_call_int_param<setCarMoving>(client, CarDirection::dirRotation);
            timerDirKey.start(k_stopTime);
            soundIntf.speak("旋转");
        } else {
            rpc_call_int_param<setCarSteps>(client, CarDirection::dirRotation, input);
            soundIntf.speak(fmt::format("旋转{}步", input));
            input = 0;
        }
        break;
    default:
        ctrllog::warn("not handled key {}", keyEvent);
        break;
    }
}
