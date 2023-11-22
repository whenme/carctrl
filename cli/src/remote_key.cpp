// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <linux/input.h>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <video/sound_intf.hpp>
#include <cli/remote_key.hpp>
#include <cli/cli_car.hpp>
#include <rpc_service/rpc_service.hpp>

RemoteKey::RemoteKey(asio::io_context& context) :
    m_keyfd(0),
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
    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();
    auto& client = cmn::getSingletonInstance<cli::CliCar>().getClient();
    int32_t num = rpc_call_int_param<getMotorNum>(client);
    std::string sound;

    static IoTimer timerDirKey(m_context, [&](const asio::error_code &e, void *ctxt) {
        RemoteKey *pKey = static_cast<RemoteKey *>(ctxt);
        rpc_call_void_param<setAllMotorState>(client, 0);
    }, this, false);

    int32_t keyEvent = 0;
    int32_t ret = getKeyEvent(&keyEvent);
    if (ret || (keyEvent == oldKey) || (keyEvent < 0) || (keyEvent > 0xFF)) {
        //no key pressed or repeated key or invalid key
        return;
    }

    if (keyEvent == oldKey) {
        if (keyEvent == RC_KEY_UP || keyEvent == RC_KEY_DOWN
            || keyEvent == RC_KEY_LEFT || keyEvent == RC_KEY_RIGHT)
        {
            timerDirKey.start(k_checkTime);
        } else { // ignore other repeated key
            return;
        }
    }

    oldKey = keyEvent;
    ctrllog::info(" key {} is pressed", keyEvent);

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
        rpc_call_void_param<setCarSteps>(client, CarDirection::dirUpDown, input);
        sound = fmt::format("前进{}步", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_DOWN:
        rpc_call_int_param<setCarSteps>(client, CarDirection::dirUpDown, -input);
        sound = fmt::format("后退{}步", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_LEFT:
        rpc_call_int_param<setCarSteps>(client, CarDirection::dirLeftRight, input);
        if (num < 4) {
            sound = fmt::format("左轮前进{}步", input);
        } else {
            sound = fmt::format("左移{}步", input);
        }
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_RIGHT:
        rpc_call_int_param<setCarSteps>(client, CarDirection::dirLeftRight, -input);
        if (num < 4) {
            sound = fmt::format("右轮前进{}步", input);
        } else {
            sound = fmt::format("右移{}步", input);
        }
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_OK:
        rpc_call_int_param<setMotorSpeedLevel>(client, input);
        ctrllog::info("set motor speed level {}", input);
        sound = fmt::format("设置速度等级{}", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_STAR:
        rpc_call_void_param<setAllMotorState>(client, 0);
        sound = fmt::format("停止");
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_POUND: //rotation
        if (num < 4) {
            sound = fmt::format("两轮轩不支持", input);
        } else {
            rpc_call_int_param<setCarSteps>(client, CarDirection::dirRotation, input);
            sound = fmt::format("旋转{}步", input);
        }
        soundIntf.speak(sound);
        input = 0;
        break;
    default:
        ctrllog::warn("not handled key {}", keyEvent);
        break;
    }
}
