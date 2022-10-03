// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <poll.h>

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_ctrl.hpp"

CarCtrl::CarCtrl(asio::io_service& io_service) :
    m_carSpeed(io_service, this),
    m_timer(io_service, timerCallback, this, true)
{
    m_timer.start(1);
}

CarCtrl::~CarCtrl()
{
    m_timer.stop();
}

int32_t CarCtrl::setCtrlSpeed(int32_t motor, int32_t speed)
{
    if ((motor >= MOTOR_MAX)
        || (speed < -MOTOR_MAX_SPEED) || (speed > MOTOR_MAX_SPEED)) {
        std::cout << "CarCtrl: error speed, speed should be -" << MOTOR_MAX_SPEED << "~+" << MOTOR_MAX_SPEED << std::endl;
        return -1;
    }

    m_ctrlSpeed[motor] = speed;
    m_currentSpeed[motor] = speed;
    m_stepSpeed = true;

    m_carSpeed.setMotorState(motor, speed);
    return 0;
}

int32_t CarCtrl::getCtrlSpeed(int32_t motor, int32_t *speed)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
        return -1;
    }
    *speed = m_ctrlSpeed[motor];
    return 0;
}

int32_t CarCtrl::getActualSpeed(int32_t motor)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
        return 0;
    }
    return m_carSpeed.getActualSpeed(motor);
}

int32_t CarCtrl::setCtrlSteps(int32_t motor, int32_t steps)
{
    auto setRunSteps = [&](int32_t wheel) {
        m_stepState[wheel] = true;
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

    if (motor > MOTOR_MAX) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
        return -1;
    }

    m_stepSpeed = false;
    if (motor == 0) {
        m_straight = true;
        setRunSteps(MOTOR_LEFT);
        setRunSteps(MOTOR_RIGHT);
    } else {
        m_straight = false;
        setRunSteps(motor - 1);
    }

    return 0;
}

int32_t CarCtrl::getCtrlSteps(int32_t motor)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
        return -1;
    }

    return m_ctrlSetSteps[motor];
}

bool CarCtrl::getCtrlState()
{
    return m_stepSpeed;
}

int32_t CarCtrl::getActualSteps(int32_t motor)
{
    if (motor >= MOTOR_MAX) {
        std::cout << "CarCtrl: error motor " << motor << std::endl;
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
            m_stepState[motor] = false;
        } else {
            int32_t diffSteps = m_ctrlSubSteps[motor] - m_carSpeed.getActualSteps(motor);
            if (m_ctrlSetSteps[motor] >= m_ctrlSteps[motor] + MOTOR_MAX_SUBSTEP)
                m_ctrlSubSteps[motor] = MOTOR_MAX_SUBSTEP + diffSteps;
            else
                m_ctrlSubSteps[motor] = m_ctrlSetSteps[motor] - m_ctrlSteps[motor];
            m_carSpeed.setRunSteps(motor, m_ctrlSubSteps[motor]);
            m_stepState[motor] = true;
        }
    };

    if ((m_stepState[MOTOR_LEFT] == true) && (m_stepState[MOTOR_RIGHT] == true)) {
        if ((m_carSpeed.getRunState(MOTOR_LEFT) == false)
            && (m_carSpeed.getRunState(MOTOR_RIGHT) == false)) { //both stopped
            for (int32_t i = 0; i < MOTOR_MAX; i++) {
                setSubSteps(i);
            }
        }
    } else if (m_stepState[MOTOR_LEFT] == true) {
        if (m_carSpeed.getRunState(MOTOR_LEFT) == false) { // left stopped
            setSubSteps(MOTOR_LEFT);
        }
    } else if (m_stepState[MOTOR_RIGHT] == true) {
        if (m_carSpeed.getRunState(MOTOR_RIGHT) == false) { // right stopped
            setSubSteps(MOTOR_RIGHT);
        }
    }

    //when all stopped, fix the last difference steps
    if (m_straight && (m_stepState[0] == false) && (m_stepState[1] == false)) {
        if (m_ctrlSteps[0] > m_ctrlSteps[1]) {
            int32_t steps = m_ctrlSteps[0] - m_ctrlSteps[1];
            m_carSpeed.setRunSteps(1, steps);
            m_stepState[1] = true;
        } else if (m_ctrlSteps[0] < m_ctrlSteps[1]) {
            int32_t steps = m_ctrlSteps[1] - m_ctrlSteps[0];
            m_carSpeed.setRunSteps(0, steps);
            m_stepState[0] = true;
        }
    }
}

void CarCtrl::timerCallback(const asio::error_code &e, void *ctxt)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(ctxt);
    static int32_t runState[] {0, 0};
    static int32_t runTime[] {0, 0};

    if (pCtrl->m_stepSpeed == false) { //step control
        pCtrl->checkNextSteps();
        return;
    }

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        int32_t currentCtrlSpeed = pCtrl->m_carSpeed.getCurrentCtrlSpeed(i);
        runTime[i]++;
        if (runState[i] == 0) {//stop
            pCtrl->m_carSpeed.setMotorState(i, 0);
            if (runTime[i] > (MOTOR_MAX_TIME - abs(currentCtrlSpeed*MOTOR_SPEED_STEP))) {
                runTime[i] = 0;
                runState[i] = 1;
            }
        } else {  //run
            if (pCtrl->m_ctrlSpeed[i] > 0) {
                pCtrl->m_carSpeed.setMotorState(i, 1);
            } else if (pCtrl->m_ctrlSpeed[i] < 0) {
                pCtrl->m_carSpeed.setMotorState(i, -1);
            }

            if (runTime[i] > abs(currentCtrlSpeed*MOTOR_SPEED_STEP)) {
                runTime[i] = 0;
                runState[i] = 0;
            }
        }
    }
}
