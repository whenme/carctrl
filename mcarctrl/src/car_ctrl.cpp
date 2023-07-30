// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <poll.h>

#include <ioapi/easylog.hpp>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
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
    if (motor >= m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return 0;
    } else {
        return m_carSpeed.getActualSpeed(motor);
    }
}

int32_t CarCtrl::setCtrlSteps(int32_t motor, int32_t steps)
{
    if (motor > m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return -1;
    }

    m_ctrlMode = CTRL_MODE_STEP;
    if (motor == 0) {
        for (int32_t i = 0; i < m_carSpeed.getMotorNum(); i++) {
            m_carSpeed.setRunSteps(i, steps);
        }
    } else {
        m_carSpeed.setRunSteps(motor-1, steps);
    }

    return 0;
}

int32_t CarCtrl::getCtrlSteps(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return 0;
    }

    return m_carSpeed.getCtrlSteps(motor);
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
        ctrllog::warn("error motor {}", motor);
        return 0;
    }

    return m_carSpeed.getActualSteps(motor);
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
    if (motor >= m_carSpeed.getMotorNum() || pwm > Motor::getMaxPwm()) {
        ctrllog::warn("error param: motor={} pwm={}", motor, pwm);
        return;
    }

    m_carSpeed.setMotorPwm(motor, pwm);
}

int32_t CarCtrl::getMotorPwm(int32_t motor)
{
    if (motor >= m_carSpeed.getMotorNum()) {
        ctrllog::warn("error motor {}", motor);
        return -1;
    }

    return m_carSpeed.getMotorPwm(motor);
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