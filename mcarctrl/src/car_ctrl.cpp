// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <poll.h>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_ctrl.hpp"

CarCtrl::CarCtrl(asio::io_service& io_service) :
    m_carSpeed(io_service, this),
    m_timer(io_service, timerCallback, this, true),
    m_runTimer(io_service, runTimeCallback, this, false)
{
    m_timer.start(1);
}

CarCtrl::~CarCtrl()
{
    m_timer.stop();
}


int32_t CarCtrl::getActualSpeed(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl::getActualSpeed: error motor " << motor << std::endl;
        return 0;
    } else {
        return m_carSpeed.getActualSpeed(motor);
    }
}

int32_t CarCtrl::setCtrlSteps(int32_t motor, int32_t steps)
{
    auto setRunSteps = [&](int32_t wheel) {
        m_runState[wheel] = true;
        m_ctrlSetSteps[wheel] = steps;
        m_ctrlSteps[wheel] = 0;
        if (abs(steps) <= MOTOR_MAX_SUBSTEP) {
            m_ctrlSubSteps[wheel] = steps;
            m_carSpeed.setRunSteps(wheel, steps);
        } else {
            m_ctrlSubSteps[wheel] = (steps > 0) ? MOTOR_MAX_SUBSTEP : -MOTOR_MAX_SUBSTEP;
            m_carSpeed.setRunSteps(wheel, m_ctrlSubSteps[wheel]);
        }
    };

    if (motor > m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
        return -1;
    }

    m_ctrlMode = CTRL_MODE_STEP;
    if (motor == 0) {
        m_straight = true;
        setRunSteps(MOTOR_FRONT_LEFT);
        setRunSteps(MOTOR_FRONT_RIGHT);
    } else {
        m_straight = false;
        setRunSteps(motor - 1);
    }

    return 0;
}

int32_t CarCtrl::getCtrlSteps(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl::getCtrlSteps: error motor " << motor << std::endl;
        return -1;
    }

    return m_ctrlSetSteps[motor];
}

int32_t CarCtrl::getCtrlMode()
{
    return m_ctrlMode;
}

int32_t CarCtrl::getMotorNum()
{
    return m_carSpeed.getMotorNum();
}

int32_t CarCtrl::getActualSteps(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl::getActualSteps: error motor " << motor << std::endl;
        return 0;
    }

    return m_ctrlSteps[motor];
}

void CarCtrl::checkNextSteps()
{
    auto setSubSteps = [&](int32_t motor) {
        m_ctrlSteps[motor] += m_carSpeed.getActualSteps(motor);
        if (std::abs(m_ctrlSteps[motor]) >= std::abs(m_ctrlSetSteps[motor])) {
            m_carSpeed.setRunSteps(motor, 0);
            m_runState[motor] = false;
        } else {
            int32_t diffSteps = m_ctrlSubSteps[motor] - m_carSpeed.getActualSteps(motor);
            if (m_ctrlSetSteps[motor] >= m_ctrlSteps[motor] + MOTOR_MAX_SUBSTEP)
                m_ctrlSubSteps[motor] = MOTOR_MAX_SUBSTEP + diffSteps;
            else
                m_ctrlSubSteps[motor] = m_ctrlSetSteps[motor] - m_ctrlSteps[motor];
            m_carSpeed.setRunSteps(motor, m_ctrlSubSteps[motor]);
            m_runState[motor] = true;
        }
    };

    if ((m_runState[MOTOR_FRONT_LEFT] == true) && (m_runState[MOTOR_FRONT_RIGHT] == true)) {
        if ((m_carSpeed.getRunState(MOTOR_FRONT_LEFT) == false)
            && (m_carSpeed.getRunState(MOTOR_FRONT_RIGHT) == false)) { //both stopped
            for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
                setSubSteps(i);
            }
        }
    } else if (m_runState[MOTOR_FRONT_LEFT] == true) {
        if (m_carSpeed.getRunState(MOTOR_FRONT_LEFT) == false) { // left stopped
            setSubSteps(MOTOR_FRONT_LEFT);
        }
    } else if (m_runState[MOTOR_FRONT_RIGHT] == true) {
        if (m_carSpeed.getRunState(MOTOR_FRONT_RIGHT) == false) { // right stopped
            setSubSteps(MOTOR_FRONT_RIGHT);
        }
    }

    //when all stopped, fix the last difference steps
    if (m_straight && (m_runState[0] == false) && (m_runState[1] == false)) {
        if (m_ctrlSteps[0] > m_ctrlSteps[1]) {
            int32_t steps = m_ctrlSteps[0] - m_ctrlSteps[1];
            m_carSpeed.setRunSteps(1, steps);
            m_runState[1] = true;
        } else if (m_ctrlSteps[0] < m_ctrlSteps[1]) {
            int32_t steps = m_ctrlSteps[1] - m_ctrlSteps[0];
            m_carSpeed.setRunSteps(0, steps);
            m_runState[0] = true;
        }
    }
}

void CarCtrl::checkNextTime()
{
}

void CarCtrl::timerCallback(const asio::error_code &e, void *ctxt)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(ctxt);

    switch(pCtrl->m_ctrlMode)
    {
    case CTRL_MODE_STEP: //step control
        pCtrl->checkNextSteps();
        break;

    case CTRL_MODE_TIME: // time control
        pCtrl->checkNextSteps();
        break;

    default: //speed control
        break;
    }
}

void CarCtrl::runTimeCallback(const asio::error_code &e, void *ctxt)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(ctxt);

    for (int32_t i = 0; i < pCtrl->m_carSpeed.getMotorNum(); i++) {
        pCtrl->m_carSpeed.setMotorState(i, MOTOR_STATE_STOP);
        pCtrl->m_runState[i] = false;
    }
    std::cout << "CarCtrl::runTimeCallback..." << std::endl;
}

int32_t CarCtrl::setRunTime(int32_t time)
{
    m_ctrlMode = CTRL_MODE_TIME;

    for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
        m_runState[i] = true;
        m_carSpeed.setActualSteps(i, 0);
        m_carSpeed.setMotorState(i, MOTOR_STATE_FORWARD);
    }

    m_runTimer.start(time * 1000);
    return 0;
}

void CarCtrl::setMotorPwm(int32_t motor, int32_t pwm)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl::setMotorPwm: error motor " << motor << std::endl;
        return;
    }
    if (pwm > Motor::getMaxPwm()) {
        std::cout << "CarCtrl::setMotorPwm: error pwm " << pwm << std::endl;
        return;
    }

    m_carSpeed.setMotorPwm(motor, pwm);
}

int32_t CarCtrl::getMotorPwm(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        std::cout << "CarCtrl::getMotorPwm: error motor " << motor << std::endl;
        return -1;
    }

    return m_carSpeed.getMotorPwm(motor);
}