// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <fstream>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <ioapi/param_json.hpp>
#include "car_speed.hpp"
#include "car_ctrl.hpp"

CarSpeed::CarSpeed(asio2::rpc_server& ioServer, CarCtrl *carCtrl) :
    m_server(ioServer),
    m_speedThread("speed thread", IoThread::ThreadPriorityNormal, CarSpeed::threadFun, this),
    m_carCtrl(carCtrl)
{
    initJsonParam();

    // start thread in cpu1
    //m_speedThread.setCpuAffinity(1);

    m_speedThread.start();
}

CarSpeed::~CarSpeed()
{
    m_timer.stop();
    m_speedThread.stop();

    for (auto& item : m_motor) {
        delete item;
    }
    m_motor.clear();
}

void CarSpeed::initJsonParam()
{
    ParamJson param("param.json");
    std::string hostname, jsonItem;
    std::ifstream ifs("/etc/hostname", std::ifstream::in);
    ifs >> hostname;
    if (hostname == "orangepioneplus")
        jsonItem = "quadcycle.";
    else
        jsonItem = "bicycle.";

    auto createMotorObject = [&](std::string jsonMotor, std::string jsonIr) {
        std::vector<uint32_t> port;
        uint32_t inputPort;
        bool outputRet = param.getJsonParam(jsonItem + jsonMotor, port);
        bool inputRet = param.getJsonParam(jsonItem + jsonIr, inputPort);
        if (outputRet && inputRet) {
            port.push_back(inputPort);
            m_motor.push_back(new Motor(port));
        } else {
            ctrllog::warn("json parameter error: {}, {}", outputRet, inputRet);
        }
    };

    auto getPwmParam = [&](std::string item) {
        std::vector<int32_t> vectVal;
        bool ret = param.getJsonParam(jsonItem + "pwm." + item, vectVal);
        if (ret)
            m_pwmVect.push_back(vectVal);
        else
            ctrllog::warn("initParam: json pwm param error");
    };

    bool ret = param.getJsonParam(jsonItem + "motor_num", m_motorNum);
    if (!ret || !m_motorNum) {
        ctrllog::error("initParam: error motor number {}...", m_motorNum);
        return;
    }
    ctrllog::info("initParam: motor number {}", m_motorNum);

    // motor defines
    createMotorObject("motor_front_left", "ir_front_left");
    createMotorObject("motor_front_right", "ir_front_right");
    if (m_motorNum == 4) {
        createMotorObject("motor_back_left", "ir_back_left");
        createMotorObject("motor_back_right", "ir_back_right");
    }

    getPwmParam("one");
    getPwmParam("two");
    getPwmParam("three");
    getPwmParam("four");
    getPwmParam("five");
    getPwmParam("six");
    getPwmParam("seven");
    getPwmParam("eight");
    getPwmParam("nine");

    //set default speed
    setMotorSpeedLevel(2);
}

int32_t CarSpeed::getActualSpeed(int32_t motor)
{
    return m_motor[motor]->m_actualSpeed;
}

void CarSpeed::setRunSteps(int32_t motor, int32_t steps)
{
    setActualSteps(motor, 0);

    m_motor[motor]->setCtrlSteps(steps);
}

bool CarSpeed::getRunState(int32_t motor)
{
    return m_motor[motor]->getRunState();
}

int32_t CarSpeed::getCtrlSteps(int32_t motor)
{
    return m_motor[motor]->getCtrlSteps();
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

void CarSpeed::motorPwmCtrl()
{
    static int32_t pwmCount[MOTOR_NUM_MAX]{0, 0, 0, 0};
    static bool runFlag[MOTOR_NUM_MAX] { true, true, true, true };

    for (int32_t i = 0; i < m_motorNum; i++) {
        if (m_motor[i]->getRunState() != MOTOR_STATE_STOP) {
            pwmCount[i]++;
            if (pwmCount[i] >= (runFlag[i] ? m_motor[i]->getRunPwm() : m_motor[i]->getStopPwm())) {
                if (runFlag[i])
                    m_motor[i]->setNowState(MOTOR_STATE_STOP);
                else
                    m_motor[i]->setNowState(m_motor[i]->getRunState());

                pwmCount[i] = 0;
                runFlag[i] = runFlag[i] ? false : true;
            }

            if ((std::abs(m_motor[i]->getActualSteps()) > std::abs(m_motor[i]->getCtrlSteps()))
                && (m_carCtrl->getCtrlMode() == CTRL_MODE_STEP)) {
                    m_motor[i]->setRunState(MOTOR_STATE_STOP);
            }
        } else {
            pwmCount[i] = 0;
            m_motor[i]->setNowState(MOTOR_STATE_STOP);
        }
    }
}

void CarSpeed::threadFun(void *ctxt)
{
    CarSpeed *obj = static_cast<CarSpeed *>(ctxt);
    struct pollfd fds[MOTOR_NUM_MAX];
    char buffer[16];

    for (int32_t i = 0; i < obj->m_motorNum; i++) {
        fds[i].fd = obj->m_motor[i]->getInputGpioFd();
        fds[i].events = POLLPRI;
    }

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        obj->motorPwmCtrl();

        if (poll(fds, obj->m_motorNum, 0) <= 0)
            continue;

        for (int32_t i = 0; i < obj->m_motorNum; i++) {
            if (fds[i].revents & POLLPRI) {
                if (lseek(fds[i].fd, 0, SEEK_SET) < 0) {
                    ctrllog::warn("threadFun: seek failed");
                    continue;
                }
                int len = read(fds[i].fd, buffer, sizeof(buffer));
                if (len < 0) {
                    ctrllog::warn("threadFun: read failed");
                    continue;
                }
                obj->m_motor[i]->m_swCounter++;

                if ((obj->m_motor[i]->getCtrlSteps() >= 0)
                    || (obj->m_carCtrl->getCtrlMode() == CTRL_MODE_TIME))
                    obj->m_motor[i]->moveActualSteps(1);
                else
                    obj->m_motor[i]->moveActualSteps(-1);
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

void CarSpeed::setMotorSpeedLevel(int32_t level)
{
    m_speedLevel = level;
    for (int32_t ii = 0; ii < getMotorNum(); ii++) {
        m_motor[ii]->setRunPwm(m_pwmVect[level][ii]);
    }
}

int32_t CarSpeed::getMotorSpeedLevel()
{
    return m_speedLevel;
}