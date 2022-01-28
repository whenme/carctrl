#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include "RemoteKey.hpp"
#include "CarCtrl.hpp"


RemoteKey *RemoteKey::m_pInstance = NULL;

RemoteKey::RemoteKey()
:m_keyfd(0)
{
    m_keyList.clear();

    //enable lirc
    system("echo '+rc-5 +nec +rc-6 +jvc +sony +rc-5-sz +sanyo +sharp +mce_kbd +xmp' > /sys/class/rc/rc0/protocols");

    //open key file
    m_keyfd = open(RC_KEY_FILE, O_RDONLY);
    if (m_keyfd < 0)
        printf("RemoteKey::%s: fail to open %s\r\n", __func__, RC_KEY_FILE); 

    if (pthread_create(&m_tipd, NULL, RemoteKey::remoteKeyThread, this) < 0) {
        printf("RemoteKey::%s: fail to create thread\r\n", __func__);
    }
}

RemoteKey::~RemoteKey()
{
    if (m_keyfd > 0)
        close(m_keyfd);

    pthread_cancel(m_tipd);
    if (m_pInstance != NULL) {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

RemoteKey *RemoteKey::getInstance()
{
    if (m_pInstance == NULL) {
        m_pInstance = new RemoteKey();
        if (m_pInstance == NULL)
            printf("RemoteKey::%s: fail to create RemoteKey\r\n", __func__);
    }

    return m_pInstance;
}


void *RemoteKey::remoteKeyThread(void *argc)
{
    RemoteKey *pKey = static_cast<RemoteKey *>(argc);
    struct input_event ev[32];

    while(1)
    {
        sleep(1);

        //read input_event
        size_t evsize = read(pKey->m_keyfd, ev, sizeof(ev));
        if (evsize < sizeof(struct input_event)) {
            printf("RemoteKey::%s: no event\r\n", __func__);
            continue;
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

    return NULL;
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
    int32_t ret, keyEvent;
    static int32_t inputSpeed = 0;
    static int32_t oldKey = 0;

    keyEvent = 0;
    ret = getKeyEvent(&keyEvent);
    if (ret || keyEvent == oldKey) {//no key pressed or repeated key
        return;
    }

    oldKey = keyEvent;
    printf("RemoteKey::%s: key %x pressed\r\n", __func__, keyEvent);
    CarCtrl::getInstance()->getCtrlSpeed(MOTOR_LEFT, &ctrlSpeed[MOTOR_LEFT]);
    CarCtrl::getInstance()->getCtrlSpeed(MOTOR_RIGHT, &ctrlSpeed[MOTOR_RIGHT]);
    switch(keyEvent)
    {
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
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, abs(ctrlSpeed[MOTOR_LEFT]));
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, abs(ctrlSpeed[MOTOR_RIGHT]));
        inputSpeed = 0;
        break;
    case RC_KEY_DOWN:
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, -1*abs(ctrlSpeed[MOTOR_LEFT]));
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, -1*abs(ctrlSpeed[MOTOR_RIGHT]));
        inputSpeed = 0;
        break;
    case RC_KEY_LEFT:
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, ctrlSpeed[MOTOR_LEFT] - 5*MOTOR_SPEED_STEP);
        inputSpeed = 0;
        break;
    case RC_KEY_RIGHT:
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, ctrlSpeed[MOTOR_RIGHT] - 5*MOTOR_SPEED_STEP);
        inputSpeed = 0;
        break;
    case RC_KEY_OK:
        ctrlSpeed[MOTOR_LEFT] = inputSpeed;
        ctrlSpeed[MOTOR_RIGHT] = inputSpeed;
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, ctrlSpeed[MOTOR_LEFT]);
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, ctrlSpeed[MOTOR_RIGHT]);
        printf("RemoteKey::%s: set motor speed left=%d, right=%d\r\n", __func__,
            ctrlSpeed[MOTOR_LEFT], ctrlSpeed[MOTOR_RIGHT]);
        inputSpeed = 0;
        break;
    case RC_KEY_STAR:
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, 0);
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, 0);
        break;
    case RC_KEY_POUND:
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_LEFT, MOTOR_MAX_SPEED);
        CarCtrl::getInstance()->setCtrlSpeed(MOTOR_RIGHT, MOTOR_MAX_SPEED);
        break;
    default:
        printf("RemoteKey::%s: not handled key 0x%x\r\n", __func__, keyEvent);
        break;
    }
}
