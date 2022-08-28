// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <linux/input.h>
#include <ioapi/cmn_singleton.hpp>
#include "remote_key.hpp"
#include "car_ctrl.hpp"

RemoteKey::RemoteKey(asio::io_service& io_service) :
    m_keyfd(0),
    m_timer(io_service, timerCallback, this, true)
{
    m_keyList.clear();

    //enable lirc
    system("echo '+rc-5 +nec +rc-6 +jvc +sony +rc-5-sz +sanyo +sharp +mce_kbd +xmp' > /sys/class/rc/rc0/protocols");

    //open key file
    m_keyfd = open(RC_KEY_FILE, O_RDONLY | O_NONBLOCK);
    if (m_keyfd < 0)
        std::cout << "RemoteKey: fail to open " << RC_KEY_FILE << std::endl;

    m_timer.start(1000);
}

RemoteKey::~RemoteKey()
{
    m_timer.stop();
    if (m_keyfd > 0)
        close(m_keyfd);
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
        std::cout << "RemoteKey: no event" << std::endl;
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
    int32_t ctrlSpeed[MOTOR_MAX] = {0, 0};
    static int32_t inputSpeed = 0;
    static int32_t oldKey = 0;
    auto& carctrl = cmn::getSingletonInstance<CarCtrl>();

    int32_t keyEvent = 0;
    int32_t ret = getKeyEvent(&keyEvent);
    if (ret || keyEvent == oldKey) {//no key pressed or repeated key
        return;
    }

    oldKey = keyEvent;
    std::cout << "RemoteKey: key " << keyEvent << " pressed" << std::endl;
    carctrl.getCtrlSpeed(MOTOR_LEFT, &ctrlSpeed[MOTOR_LEFT]);
    carctrl.getCtrlSpeed(MOTOR_RIGHT, &ctrlSpeed[MOTOR_RIGHT]);

    switch(keyEvent) {
    case RC_KEY_0:
        inputSpeed = inputSpeed*10;
        break;
    case RC_KEY_1:
        inputSpeed = inputSpeed*10 + 1;
        break;
    case RC_KEY_2:
        inputSpeed = inputSpeed*10 + 2;
        break;
    case RC_KEY_3:
        inputSpeed = inputSpeed*10 + 3;
        break;
    case RC_KEY_4:
        inputSpeed = inputSpeed*10 + 4;
        break;
    case RC_KEY_5:
        inputSpeed = inputSpeed*10 + 5;
        break;
    case RC_KEY_6:
        inputSpeed = inputSpeed*10 + 6;
        break;
    case RC_KEY_7:
        inputSpeed = inputSpeed*10 + 7;
        break;
    case RC_KEY_8:
        inputSpeed = inputSpeed*10 + 8;
        break;
    case RC_KEY_9:
        inputSpeed = inputSpeed*10 + 9;
        break;

    case RC_KEY_UP:
        carctrl.setCtrlSpeed(MOTOR_LEFT, abs(ctrlSpeed[MOTOR_LEFT]));
        carctrl.setCtrlSpeed(MOTOR_RIGHT, abs(ctrlSpeed[MOTOR_RIGHT]));
        inputSpeed = 0;
        break;
    case RC_KEY_DOWN:
        carctrl.setCtrlSpeed(MOTOR_LEFT, -1*abs(ctrlSpeed[MOTOR_LEFT]));
        carctrl.setCtrlSpeed(MOTOR_RIGHT, -1*abs(ctrlSpeed[MOTOR_RIGHT]));
        inputSpeed = 0;
        break;
    case RC_KEY_LEFT:
        carctrl.setCtrlSpeed(MOTOR_LEFT, ctrlSpeed[MOTOR_LEFT] - 5*MOTOR_SPEED_STEP);
        inputSpeed = 0;
        break;
    case RC_KEY_RIGHT:
        carctrl.setCtrlSpeed(MOTOR_RIGHT, ctrlSpeed[MOTOR_RIGHT] - 5*MOTOR_SPEED_STEP);
        inputSpeed = 0;
        break;
    case RC_KEY_OK:
        ctrlSpeed[MOTOR_LEFT] = inputSpeed;
        ctrlSpeed[MOTOR_RIGHT] = inputSpeed;
        carctrl.setCtrlSpeed(MOTOR_LEFT, ctrlSpeed[MOTOR_LEFT]);
        carctrl.setCtrlSpeed(MOTOR_RIGHT, ctrlSpeed[MOTOR_RIGHT]);
        std::cout << "RemoteKey: set motor speed left=" << ctrlSpeed[MOTOR_LEFT]
                  << " right=" << ctrlSpeed[MOTOR_RIGHT] << std::endl;
        inputSpeed = 0;
        break;
    case RC_KEY_STAR:
        carctrl.setCtrlSpeed(MOTOR_LEFT, 0);
        carctrl.setCtrlSpeed(MOTOR_RIGHT, 0);
        break;
    case RC_KEY_POUND:
        carctrl.setCtrlSpeed(MOTOR_LEFT, MOTOR_MAX_SPEED);
        carctrl.setCtrlSpeed(MOTOR_RIGHT, MOTOR_MAX_SPEED);
        break;
    default:
        std::cout << "RemoteKey: not handled key " << keyEvent << std::endl;
        break;
    }
}
