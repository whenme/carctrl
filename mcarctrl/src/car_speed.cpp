// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <fstream>

#include <xapi/cmn_singleton.hpp>
#include <xapi/easylog.hpp>
#include <xapi/param_json.hpp>
#include "car_speed.hpp"
#include "car_ctrl.hpp"

CarSpeed::CarSpeed(asio::io_context& context, CarCtrl *carCtrl) :
    m_context{context},
    m_speedThread{"speed thread", cmn::CmnThread::ThreadPriorityNormal, CarSpeed::threadFun, this},
    m_carCtrl{carCtrl}
{
    initJsonParam();

    // start thread in cpu1
    //m_speedThread.setCpuAffinity(1);

    m_speedThread.start();
}

CarSpeed::~CarSpeed()
{
    m_speedThread.stop();

    for (auto& item : m_motor) {
        delete item;
    }
    m_motor.clear();

    if (m_steer) {
        delete m_steer;
        m_steer = nullptr;
    }
}

void CarSpeed::initJsonParam()
{
    ParamJson param("param.json");
    std::string jsonItem;
    std::ifstream ifs("/etc/hostname", std::ifstream::in);
    ifs >> jsonItem;

    if (jsonItem.find("pi") == std::string::npos) {
        jsonItem = k_deviceNamePc;
    }

    auto createMotorObject = [&](std::string jsonMotor, std::string jsonIr) {
        std::vector<uint32_t> port;
        uint32_t inputPort;
        bool outputRet = param.getJsonParam(jsonItem + "." + jsonMotor, port);
        bool inputRet = param.getJsonParam(jsonItem + "." + jsonIr, inputPort);
        if (outputRet && inputRet) {
            port.push_back(inputPort);
            m_motor.push_back(new Motor(port));
        } else if (outputRet) {
            ctrllog::warn("create motor port {}, {}", port[0], port[1]);
            m_motor.push_back(new Motor(port));
            ctrllog::warn("create motor with port {},{}", port[0], port[1]);
        } else {
            ctrllog::warn("json parameter error: {}, {}", outputRet, inputRet);
        }
    };

    auto getPwmParam = [&](std::string item) {
        std::vector<int32_t> vectVal;
        bool ret = param.getJsonParam(jsonItem + ".pwm." + item, vectVal);
        if (ret)
            m_pwmVect.push_back(vectVal);
        else
            ctrllog::warn("initParam: json pwm param error");
    };

    bool ret = param.getJsonParam(jsonItem + ".motor_num", m_motorNum);
    if (!ret || !m_motorNum) {
        ctrllog::error("initParam: error motor number {}...", m_motorNum);
        return;
    }
    ctrllog::info("initParam: motor number {}, jsonItem {}", m_motorNum, jsonItem);

    if (jsonItem == k_deviceNameM1) {
        // motor defines
        createMotorObject("motor_front_left", "ir_front_left");
        createMotorObject("motor_front_right", "ir_front_right");
        if (m_motorNum == 4) {
            createMotorObject("motor_back_left", "ir_back_left");
            createMotorObject("motor_back_right", "ir_back_right");
        }
    } else if (jsonItem == k_deviceNamePc) {
        createMotorObject("motor_front", "none");
        createMotorObject("motor_back", "none");
        std::vector<uint32_t> port;
        bool steerRet = param.getJsonParam(jsonItem + ".steer", port);
        if (steerRet) {
            m_steer = new Steer(m_context, port);
            if (m_steer == nullptr) {
                ctrllog::error("failed to create steer...");
            }
        } else {
            ctrllog::warn("no steer parameter...");
        }
    }

    getPwmParam("zero");
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
    setMotorSpeedLevel(1);
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

MotorState CarSpeed::getRunState(int32_t motor)
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

void CarSpeed::motorPwmCtrl()
{
    static int32_t pwmCount[MOTOR_NUM_MAX]{0, 0, 0, 0};
    static bool runFlag[MOTOR_NUM_MAX] { true, true, true, true };

    for (int32_t i = 0; i < m_motorNum; i++) {
        if (m_motor[i]->getRunState() != MotorState::Stop) {
            pwmCount[i]++;
            if (pwmCount[i] >= (runFlag[i] ? m_motor[i]->getRunPwm() : m_motor[i]->getStopPwm())) {
                if (runFlag[i])
                    m_motor[i]->setNowState(MotorState::Stop);
                else
                    m_motor[i]->setNowState(m_motor[i]->getRunState());

                pwmCount[i] = 0;
                runFlag[i] = runFlag[i] ? false : true;
            }

            if ((std::abs(m_motor[i]->getActualSteps()) > std::abs(m_motor[i]->getCtrlSteps()))
                && (m_carCtrl->getCtrlMode() == CTRL_MODE_STEP)) {
                    m_motor[i]->setRunState(MotorState::Stop);
            }
        } else {
            pwmCount[i] = 0;
            m_motor[i]->setNowState(MotorState::Stop);
        }
    }
}

void CarSpeed::threadFun(void *ctxt)
{
    CarSpeed *obj = static_cast<CarSpeed *>(ctxt);
    struct pollfd fds[MOTOR_NUM_MAX]{};
    char buffer[16];
    bool inputFlag {false};

    for (int32_t ii = 0; ii < obj->m_motorNum; ii++) {
        if (obj->m_motor[ii]->getInputGpioFd()) {
            fds[ii].fd = obj->m_motor[ii]->getInputGpioFd();
            fds[ii].events = POLLPRI;
            inputFlag = true;
        }
    }

    while(1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        obj->motorPwmCtrl();

        if (inputFlag) { //with input counter
            if (poll(fds, obj->m_motorNum, 0) <= 0) {
                continue;
            }

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
                        || (obj->m_carCtrl->getCtrlMode() == CTRL_MODE_TIME)) {
                        obj->m_motor[i]->moveActualSteps(1);
                    } else {
                        obj->m_motor[i]->moveActualSteps(-1);
                    }
                }
            }
        }
    }
}

void CarSpeed::setMotorState(int32_t motor, MotorState state)
{
    m_motor[motor]->setRunState(state);
}

MotorState CarSpeed::getMotorState(int32_t motor)
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

void CarSpeed::steerTurn(int32_t dir, uint32_t time)
{
    if (m_steer) {
        m_steer->turn(dir, time);
    }
}
