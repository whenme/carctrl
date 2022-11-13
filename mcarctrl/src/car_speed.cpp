// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <fstream>

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
    initJsonParam();

    // start thread in cpu1
    //m_speedThread.setCpuAffinity(1);

    m_timerSpeed.start(1000);
    m_speedThread.start();
}

CarSpeed::~CarSpeed()
{
    m_timerSpeed.stop();
    m_speedThread.stop();

    for (auto& item : m_motor) {
        delete item;
    }
    m_motor.clear();
}

void CarSpeed::initJsonParam()
{
    std::string filename{"param.json"};
    ParamJson param(filename);

    std::string hostname, jsonItem;
    std::ifstream ifs("/etc/hostname", std::ifstream::in);
    ifs >> hostname;
    if (hostname == "orangepioneplus")
        jsonItem = "quadcycle.";
    else
        jsonItem = "tricycle.";

    auto createMotorObject = [&](std::string jsonMotor, std::string jsonIr) {
        std::vector<uint32_t> port;
        uint32_t inputPort;
		int32_t outputRet = param.getJsonParam(jsonItem + jsonMotor, port);
        int32_t inputRet = param.getJsonParam(jsonItem + jsonIr, inputPort);
        if (outputRet && inputRet) {
            port.push_back(inputPort);
            m_motor.push_back(new Motor(port));
        } else {
            std::cout << "CarSpeed::initParam: json parameter error: " << outputRet << "," << inputRet << std::endl;
        }
    };

    bool ret = param.getJsonParam(jsonItem + "motor_num", m_motorNum);
    if (!ret || !m_motorNum) {
        std::cout << "CarSpeed::initParam: error motor number " << m_motorNum << "..." << std::endl;
        return;
    }
    std::cout << "CarSpeed::initParam: motor number " << m_motorNum << std::endl;

    // motor defines
    createMotorObject("motor_front_left", "ir_front_left");
    createMotorObject("motor_front_right", "ir_front_right");
    if (m_motorNum == 4) {
        createMotorObject("motor_back_left", "ir_back_left");
        createMotorObject("motor_back_right", "ir_back_right");
    }
}

int32_t CarSpeed::getActualSpeed(int32_t motor)
{
    return m_motor[motor]->m_actualSpeed;
}

void CarSpeed::setRunSteps(int32_t motor, int32_t steps)
{
    setActualSteps(motor, 0);
    m_ctrlSetSteps[motor] = steps;

    if (steps > 10)
        m_motor[motor]->setCtrlSteps(steps - 4);
    else if (steps > 5)
        m_motor[motor]->setCtrlSteps(steps - 2);
    else if ((steps >= -5) && (steps <= 5))
        m_motor[motor]->setCtrlSteps(steps);
    else if ((steps >= -10) && (steps < -5))
        m_motor[motor]->setCtrlSteps(steps + 2);
    else if (steps < -10)
        m_motor[motor]->setCtrlSteps(steps + 4);

    m_motor[motor]->setRunState(steps);
}

bool CarSpeed::getRunState(int32_t motor)
{
    return m_motor[motor]->getRunState();
}

int32_t CarSpeed::getActualSteps(int32_t motor)
{
    return m_motor[motor]->getActualSteps();
}

void CarSpeed::setActualSteps(int32_t motor, int32_t steps)
{
    m_motor[motor]->setActualSteps(steps);
}

void CarSpeed::timerSpeedCallback(const asio::error_code &e, void *ctxt)
{
    CarSpeed* obj = static_cast<CarSpeed*>(ctxt);
    static int32_t counter[] {0, 0, 0, 0};

    for (int32_t i = 0; i < obj->m_motorNum; i++) {
        if (obj->m_motor[i]->getRunState() != MOTOR_STATE_STOP) {
            obj->m_motor[i]->m_actualSpeed = obj->m_motor[i]->m_swCounter - counter[i];
            counter[i] = obj->m_motor[i]->m_swCounter;
        }
    }
}

void CarSpeed::threadFun(void *ctxt)
{
    CarSpeed *obj = static_cast<CarSpeed *>(ctxt);
    struct pollfd fds[MOTOR_NUM_MAX];
    char buffer[16];
    int32_t i, pwmCount[MOTOR_NUM_MAX]{0, 0, 0, 0};
    bool runFlag[MOTOR_NUM_MAX] { true, true, true, true };

    for (i = 0; i < obj->m_motorNum; i++) {
        fds[i].fd = obj->m_motor[i]->getInputGpioFd();
        fds[i].events = POLLPRI;
    }

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        for (i = 0; i < obj->m_motorNum; i++) {
            if (obj->m_motor[i]->getRunState() != MOTOR_STATE_STOP) {
                pwmCount[i]++;
                if (pwmCount[i] >= (runFlag[i] ? obj->m_motor[i]->getRunPwm() : obj->m_motor[i]->getStopPwm())) {
                    pwmCount[i] = 0;
                    if (runFlag[i])
                        obj->m_motor[i]->setRunState(MOTOR_STATE_STOP);
                    else
                        obj->m_motor[i]->setRunState(obj->m_motor[i]->getCtrlSteps() > 0?MOTOR_STATE_FORWARD:MOTOR_STATE_BACK);
                    runFlag[i] = runFlag[i] ? false : true;
                }
            }
        }

        if (poll(fds, obj->m_motorNum, 0) <= 0)
            continue;

        for (i = 0; i < obj->m_motorNum; i++) {
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
                obj->m_motor[i]->m_swCounter++;

                if (obj->m_carCtrl->getCtrlMode() == CTRL_MODE_STEP) { //ctrl is step
                    if (obj->m_motor[i]->getCtrlSteps() >= 0)
                        obj->m_motor[i]->getActualSteps()++;
                    else
                        obj->m_motor[i]->getActualSteps()--;
                    if (std::abs(obj->m_motor[i]->getActualSteps()) > std::abs(obj->m_motor[i]->getCtrlSteps())) {
                        obj->m_motor[i]->setRunState(MOTOR_STATE_STOP);
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

int32_t CarSpeed::getMotorNum()
{
    return m_motorNum;
}

void CarSpeed::setMotorPwm(int32_t motor, int32_t pwm)
{
    m_motor[motor]->setRunPwm(pwm);
}
	
int32_t CarSpeed::getMotorPwm(int32_t motor)
{
    return m_motor[motor]->getRunPwm();
}