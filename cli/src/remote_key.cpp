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
    m_timer(context, timerCallback, this, true)
{
    m_keyList.clear();

    initIrKey();
    m_timer.start(1000);
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
    int evsize = read(pKey->m_keyfd, ev, sizeof(ev));
    if (evsize < 0)
        return;

    if (evsize < sizeof(struct input_event)) {
        ctrllog::info("no event");
        return;
    }

    for (int32_t i = 0; i < evsize/sizeof(struct input_event); i++) {
        if (4 == ev[i].code) { //only handle key pressed
            if (ev[i].value != 0) {
                pKey->m_keyList.push_back(ev[i].value);
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
    char sound[128] {0};

    int32_t keyEvent = 0;
    int32_t ret = getKeyEvent(&keyEvent);
    if (ret || (keyEvent == oldKey) || (keyEvent < 0) || (keyEvent > 0xFF)) {
        //no key pressed or repeated key or invalid key
        return;
    }

    oldKey = keyEvent;
    ctrllog::info(" key {} is pressed", keyEvent);
    //std::cout << "RemoteKey: key " << std::hex << keyEvent << " pressed" << std::dec << std::endl;

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
        rpc_call_void_param<setCtrlSteps>(client, 0, input);
        sprintf(sound, "前进%d步", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_DOWN:
        rpc_call_int_param<setCtrlSteps>(client, 0, -input);
        sprintf(sound, "后退%d步", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_LEFT:
        if (num < 4) {
            rpc_call_int_param<setCtrlSteps>(client, 1, input);
            sprintf(sound, "左轮前进%d步", input);
        } else {
            rpc_call_int_param<setCtrlSteps>(client, 1, -input);
            rpc_call_int_param<setCtrlSteps>(client, 2, input);
            rpc_call_int_param<setCtrlSteps>(client, 3, input);
            rpc_call_int_param<setCtrlSteps>(client, 4, -input);
            sprintf(sound, "左移%d步", input);
        }
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_RIGHT:
        if (num < 4) {
            rpc_call_void_param<setCtrlSteps>(client, 2, input);
            sprintf(sound, "右轮前进%d步", input);
        } else {
            rpc_call_int_param<setCtrlSteps>(client, 1, input);
            rpc_call_int_param<setCtrlSteps>(client, 2, -input);
            rpc_call_int_param<setCtrlSteps>(client, 3, -input);
            rpc_call_int_param<setCtrlSteps>(client, 4, input);
            sprintf(sound, "右移%d步", input);
        }
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_OK:
        rpc_call_int_param<setMotorSpeedLevel>(client, input);
        ctrllog::info("set motor speed level {}", input);
        sprintf(sound, "设置速度等级%d", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_STAR:
        rpc_call_void_param<setAllMotorState>(client, 0);
        sprintf(sound, "停止");
        soundIntf.speak(sound);
        input = 0;
        break;
    case RC_KEY_POUND: //rotation
        rpc_call_int_param<setCtrlSteps>(client, 1, input);
        rpc_call_int_param<setCtrlSteps>(client, 2, -input);
        rpc_call_int_param<setCtrlSteps>(client, 3, input);
        rpc_call_int_param<setCtrlSteps>(client, 4, -input);
        sprintf(sound, "旋转%d步", input);
        soundIntf.speak(sound);
        input = 0;
        break;
    default:
        ctrllog::warn("not handled key {}", keyEvent);
        break;
    }
}
