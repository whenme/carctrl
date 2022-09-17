// SPDX-License-Identifier: GPL-2.0
#include <poll.h>
#include <iostream>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_speed.hpp"
#include "car_ctrl.hpp"

CarSpeed::CarSpeed(asio::io_service& io_service, CarCtrl *carCtrl) :
    m_ioService(io_service),
    m_timerSpeed(io_service, timerSpeedCallback, this, true),
    m_carCtrl(carCtrl)
{
    std::string filename{"car.json"};
    ParamJson param(filename);
    uint32_t left, right;

    bool ret = param.getJsonParam("car.ir_left", left);
    if (ret) {
        m_gpioSpeed[MOTOR_LEFT] = new Gpio(left, GPIO_DIR_IN, GPIO_EDGE_RISING);
    }

    ret = param.getJsonParam("car.ir_right", right);
    if (ret) {
        m_gpioSpeed[MOTOR_RIGHT] = new Gpio(right, GPIO_DIR_IN, GPIO_EDGE_RISING);
    }

    if ((m_gpioSpeed[MOTOR_LEFT] == nullptr) || (m_gpioSpeed[MOTOR_RIGHT] == nullptr)) {
        std::cout << "CarSpeed: fail to create gpio object" << std::endl;
    }

    if (pthread_create(&m_tipd, nullptr, CarSpeed::carSpeedThread, this) < 0) {
        std::cout << "CarSpeed: fail to create thread" << std::endl;
    }

    m_timerSpeed.start(1000);
}

CarSpeed::~CarSpeed()
{
    m_timerSpeed.stop();
    pthread_cancel(m_tipd);

    delete m_gpioSpeed[MOTOR_LEFT];
    delete m_gpioSpeed[MOTOR_RIGHT];
}

int32_t CarSpeed::getActualSpeed(int32_t motor)
{
    return m_actualSpeed[motor];
}

void CarSpeed::setRunSteps(int32_t motor, int32_t steps)
{
    m_ctrlSetSteps[motor] = steps;
    if (steps > 10)
        m_ctrlSteps[motor] = steps - 4;
    else if (steps > 5)
        m_ctrlSteps[motor] = steps - 2;
    else if ((steps >= -5) && (steps <= 5))
        m_ctrlSteps[motor] = steps;
    else if ((steps >= -10) && (steps < -5))
        m_ctrlSteps[motor] = steps + 2;
    else if (steps < -10)
        m_ctrlSteps[motor] = steps + 4;

    m_actualSteps[motor] = 0;
    m_runState[motor] = true;
}

bool CarSpeed::getRunState(int32_t motor)
{
    return m_runState[motor];
}

int32_t CarSpeed::getActualSteps(int32_t motor)
{
    return m_actualSteps[motor];
}

void CarSpeed::calculateSpeedCtrl()
{
    int32_t speed, diffSpeed, ctrlSpeed;
    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        m_carCtrl->getCtrlSpeed(i, &ctrlSpeed);
        speed = getActualSpeed(i);
        diffSpeed = ctrlSpeed - speed;
        if (diffSpeed >= 0) { //speed is low to ctrl
            if (ctrlSpeed >= 0)
                m_currentCtrlSpeed[i] += abs(diffSpeed)/2;
            else
                m_currentCtrlSpeed[i] -= abs(diffSpeed)/2;
        } else {    //speed is high to ctrl
            if (ctrlSpeed >= 0)
                m_currentCtrlSpeed[i] -= abs(diffSpeed)/2;
            else
                m_currentCtrlSpeed[i] += abs(diffSpeed)/2;
        }
    }
}

int32_t CarSpeed::getCurrentCtrlSpeed(int32_t motor)
{
     return m_currentCtrlSpeed[motor];
}

void CarSpeed::timerSpeedCallback(const asio::error_code &e, void *ctxt)
{
    CarSpeed* speedObj = static_cast<CarSpeed*>(ctxt);
    static int32_t counter[] {0, 0};

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        speedObj->m_actualSpeed[i] = speedObj->m_swCounter[i] - counter[i];
        counter[i] = speedObj->m_swCounter[i];
    }

    speedObj->calculateSpeedCtrl();
}

void *CarSpeed::carSpeedThread(void *args)
{
    CarSpeed *obj = static_cast<CarSpeed *>(args);
    struct pollfd fds[MOTOR_MAX];
    char buffer[16];

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        fds[i].fd = obj->m_gpioSpeed[i]->getGpioFd();
        fds[i].events = POLLPRI;
    }

    while(1) {
        usleep(1000);
        if (poll(fds, MOTOR_MAX, 0) < 0) {
            std::cout << "CarSpeed: fail to poll" << std::endl;
            continue;
        }

        for (int32_t i = 0; i < MOTOR_MAX; i++) {
            if (fds[i].revents & POLLPRI) {
                if (lseek(fds[i].fd, 0, SEEK_SET) < 0) {
                    std::cout << "CarSpeed: seek failed" << std::endl;
                    continue;
                }
                int len = read(fds[i].fd, buffer, sizeof(buffer));
                if (len < 0) {
                    std::cout << "CarSpeed: read failed" << std::endl;
                    continue;
                }
                obj->m_swCounter[i]++;
                if (obj->m_ctrlSteps[i] >= 0)
                    obj->m_actualSteps[i]++;
                else
                    obj->m_actualSteps[i]--;
                if (std::abs(obj->m_actualSteps[i]) >= std::abs(obj->m_ctrlSteps[i])) {
                    obj->m_carCtrl->setCarState(i, 0);
                    obj->m_runState[i] = false;
                }
            }
        }
    }
    
    return NULL;
}
