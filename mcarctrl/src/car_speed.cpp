// SPDX-License-Identifier: GPL-2.0
#include <iostream>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_speed.hpp"
#include "car_ctrl.hpp"

CarSpeed::CarSpeed(asio::io_service& io_service, CarCtrl *carCtrl) :
    m_ioService(io_service),
    m_timerSpeed(m_ioService, timerSpeedCallback, this, true),
    m_speedThread("speed thread", IoThread::ThreadPriorityNormal, CarSpeed::threadFun, this),
    m_carCtrl(carCtrl)
{
    std::string filename{"car.json"};
    ParamJson param(filename);
    uint32_t left, right;
    std::vector<uint32_t> leftPort, rightPort;

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

    ret = param.getJsonParam("car.wheel_port_left", leftPort);
    if (ret) {
        m_motor[MOTOR_LEFT] = new Motor(leftPort);
    }
    ret = param.getJsonParam("car.wheel_port_right", rightPort);
    if (ret) {
        m_motor[MOTOR_RIGHT] = new Motor(rightPort);
    }

    if ((m_motor[MOTOR_LEFT] == nullptr) || (m_motor[MOTOR_RIGHT] == nullptr)) {
        std::cout << "CarSpeed: fail to create motor object" << std::endl;
    }

    // start thread in cpu1
    m_speedThread.start();
    m_speedThread.setCpuAffinity(1);

    m_timerSpeed.start(1000);
    m_speedThread.start();
}

CarSpeed::~CarSpeed()
{
    m_timerSpeed.stop();
    m_speedThread.stop();

    delete m_gpioSpeed[MOTOR_LEFT];
    delete m_gpioSpeed[MOTOR_RIGHT];
    delete m_motor[MOTOR_LEFT];
    delete m_motor[MOTOR_RIGHT];
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
    m_motor[motor]->setRunState(steps);
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

void CarSpeed::threadFun(void *ctxt)
{
    CarSpeed *obj = static_cast<CarSpeed *>(ctxt);
    struct pollfd fds[MOTOR_MAX];
    char buffer[16];
    int32_t i, pwmCount[MOTOR_MAX]{0, 0};
    bool runFlag[MOTOR_MAX] { true, true };

    for (i = 0; i < MOTOR_MAX; i++) {
        fds[i].fd = obj->m_gpioSpeed[i]->getGpioFd();
        fds[i].events = POLLPRI;
    }

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        for (i = 0; i < MOTOR_MAX; i++) {
            if (obj->m_runState[i] == true) {
                pwmCount[i]++;
                if (pwmCount[i] >= (runFlag[i] ? obj->m_ctrlPwm[i] : (obj->m_maxPwm[i] - obj->m_ctrlPwm[i]))) {
                    pwmCount[i] = 0;
                    if (runFlag[i])
                        obj->m_motor[i]->setRunState(MOTOR_STATE_STOP);
                    else
                        obj->m_motor[i]->setRunState(obj->m_ctrlSteps[i] > 0?MOTOR_STATE_FORWARD:MOTOR_STATE_BACK);
                    runFlag[i] = runFlag[i] ? false : true;
                }
            }
        }

        if (poll(fds, MOTOR_MAX, 0) <= 0)
            continue;

        for (i = 0; i < MOTOR_MAX; i++) {
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

                if (!obj->m_carCtrl->getCtrlState()) { //ctrl is step
                    if (obj->m_ctrlSteps[i] >= 0)
                        obj->m_actualSteps[i]++;
                    else
                        obj->m_actualSteps[i]--;
                    if (std::abs(obj->m_actualSteps[i]) >= std::abs(obj->m_ctrlSteps[i])) {
                        obj->m_motor[i]->setRunState(MOTOR_STATE_STOP);
                        obj->m_runState[i] = false;
                    }
                }
            }
        }
    }
}

void CarSpeed::setMotorState(int32_t motor, int32_t state)
{
    m_motor[motor]->setRunState(state);
}

int32_t CarSpeed::getMotorState(int32_t motor)
{
    return m_motor[motor]->getRunState();
}