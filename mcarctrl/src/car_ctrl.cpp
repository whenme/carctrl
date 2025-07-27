// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <poll.h>

#include <xapi/easylog.hpp>
#include <xapi/cmn_singleton.hpp>
#include <xapi/param_json.hpp>
#include "car_ctrl.hpp"

CarCtrl::CarCtrl(asio::io_context& context) :
    m_carSpeed(context, this),
    m_runTimer(context, runTimeCallback, this, false)
{
}

CarCtrl::~CarCtrl()
{
    m_runTimer.stop();
}


int32_t CarCtrl::getActualSpeed(int32_t motor)
{
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return 0;
    } else {
        return m_carSpeed.getActualSpeed(motor-1);
    }
}

int32_t CarCtrl::setCtrlSteps(int32_t motor, int32_t steps)
{
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return -1;
    }

    if (!steps) {
        if (motor == 0) {
            for (int32_t ii = 0; ii < m_carSpeed.getMotorNum(); ii++) {
                m_carSpeed.setMotorState(ii, MOTOR_STATE_STOP);
            }
        } else {
            m_carSpeed.setMotorState(motor-1, MOTOR_STATE_STOP);
        }
        return 0;
    }

    //revise steps
    int32_t level = m_carSpeed.getMotorSpeedLevel();
    if (steps > 10)
        steps -= level;
    else if ((steps > 0) && (steps <= 10))
        steps -= level/2;
    else if ((steps < 0) && (steps >= -10))
        steps += level/2;
    else
        steps += level;

    m_ctrlMode = CTRL_MODE_STEP;
    if (motor == 0) {
        for (int32_t ii = 0; ii < m_carSpeed.getMotorNum(); ii++) {
            m_carSpeed.setRunSteps(ii, steps);
        }
    } else {
        m_carSpeed.setRunSteps(motor-1, steps);
    }

    return 0;
}

int32_t CarCtrl::getCtrlSteps(int32_t motor)
{
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return 0;
    }

    return m_carSpeed.getCtrlSteps(motor-1);
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
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return 0;
    }

    return m_carSpeed.getActualSteps(motor-1);
}

void CarCtrl::runTimeCallback(const asio::error_code &e, void *ctxt)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(ctxt);

    pCtrl->setAllMotorState(MOTOR_STATE_STOP);
}

int32_t CarCtrl::setRunTime(int32_t time)
{
    m_ctrlMode = CTRL_MODE_TIME;

    for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
        m_carSpeed.setActualSteps(i, 0);
    }
    setAllMotorState(MOTOR_STATE_FORWARD);
    m_runTimer.start(time * 1000);

    return 0;
}

void CarCtrl::setMotorPwm(int32_t motor, int32_t pwm)
{
    if (motor > m_carSpeed.getMotorNum() || pwm > Motor::getMaxPwm()) {
        ctrllog::warn("error param: motor={} pwm={}", motor, pwm);
        return;
    }

    m_carSpeed.setMotorPwm(motor-1, pwm);
}

int32_t CarCtrl::getMotorPwm(int32_t motor)
{
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return -1;
    }

    return m_carSpeed.getMotorPwm(motor-1);
}

int32_t CarCtrl::setMotorSpeedLevel(int32_t level)
{
    if ((level < 1) || (level > 9)) {
        ctrllog::warn("speed level error. level <1-9>");
        return -1;
    }
    ctrllog::warn("CarCtrl::setMotorSpeedLevel {}", level);
    m_carSpeed.setMotorSpeedLevel(level - 1);
    return 0;
}

int32_t CarCtrl::getMotorSpeedLevel()
{
    return m_carSpeed.getMotorSpeedLevel() + 1;
}

void CarCtrl::setAllMotorState(int32_t state)
{
    for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
        m_carSpeed.setMotorState(i, state);
    }
}

int32_t CarCtrl::setCarSteps(CarDirection dir, int32_t steps)
{
    switch (dir) {
    case CarDirection::dirUp:
        setCtrlSteps(0, steps);
        break;
    case CarDirection::dirLeft:
        if (m_carSpeed.getMotorNum() == 2) {
            if (steps > 0)
                setCtrlSteps(1, steps);
            else
                setCtrlSteps(2, -steps);
        } else {
            setCtrlSteps(1, -steps);
            setCtrlSteps(2, steps);
            setCtrlSteps(3, steps);
            setCtrlSteps(4, -steps);
        }
        break;
    case CarDirection::dirRotation:
        if (m_carSpeed.getMotorNum() == 2) {
            ctrllog::warn("two motor not support");
            return -1;
        } else {
            setCtrlSteps(1, steps);
            setCtrlSteps(2, -steps);
            setCtrlSteps(3, steps);
            setCtrlSteps(4, -steps);
        }
        break;
    default:
        break;
    }

    return 0;
}

int32_t CarCtrl::setCarMoving(CarDirection dir)
{
    m_ctrlMode = CTRL_MODE_TIME;
    switch (dir) {
    case CarDirection::dirUp:
        for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
            m_carSpeed.setMotorState(i, 1);
        }
        break;
    case CarDirection::dirDown:
        for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
            m_carSpeed.setMotorState(i, -1);
        }
        break;
    case CarDirection::dirLeft:
        if (m_carSpeed.getMotorNum() == 2) {
            m_carSpeed.setMotorState(0, 1);
        } else {
            m_carSpeed.setMotorState(0, -1);
            m_carSpeed.setMotorState(1, 1);
            m_carSpeed.setMotorState(2, 1);
            m_carSpeed.setMotorState(3, -1);
        }
        break;
    case CarDirection::dirRight:
        if (m_carSpeed.getMotorNum() == 2) {
            m_carSpeed.setMotorState(1, 1);
        } else {
            m_carSpeed.setMotorState(0, 1);
            m_carSpeed.setMotorState(1, -1);
            m_carSpeed.setMotorState(2, -1);
            m_carSpeed.setMotorState(3, 1);
        }
        break;
    case CarDirection::dirRotation:
        if (m_carSpeed.getMotorNum() == 2) {
            ctrllog::warn("two motor not support");
            return -1;
        } else {
            m_carSpeed.setMotorState(0, 1);
            m_carSpeed.setMotorState(1, -1);
            m_carSpeed.setMotorState(2, 1);
            m_carSpeed.setMotorState(3, -1);
        }
        break;
    default:
        ctrllog::warn("not supported direction");
        break;
    }

    return 0;
}

int32_t CarCtrl::steerTurn(int32_t time)
{
    m_carSpeed.steerTurn(time);
    return 0;
}
