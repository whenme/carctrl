// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/param_json.hpp>
#include "car_ctrl.hpp"
#include "car_speed.hpp"

CarCtrl::CarCtrl(asio::io_service& io_service) :
    m_timer(io_service, timerCallback, this, true)
{
    std::string filename{"car.json"};
    ParamJson param(filename);
    std::vector<uint32_t> left, right;

    bool ret = param.getJsonParam("car.wheel_port_left", left);
    if (ret) {
       m_motorGpio[MOTOR_LEFT][0] = new Gpio(left.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
       m_motorGpio[MOTOR_LEFT][1] = new Gpio(left.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    }

    ret = param.getJsonParam("car.wheel_port_right", right);
    if (ret) {
       m_motorGpio[MOTOR_RIGHT][0] = new Gpio(right.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
       m_motorGpio[MOTOR_RIGHT][1] = new Gpio(right.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    }

    if ((m_motorGpio[MOTOR_LEFT][0] == NULL) || (m_motorGpio[MOTOR_LEFT][1] == NULL)
        || (m_motorGpio[MOTOR_RIGHT][0] == NULL) || (m_motorGpio[MOTOR_RIGHT][1] == NULL)) {
        std::cout <<"CarCtrl: fail to create motor gpio" << std::endl;
    }

    m_timer.start(1);
}

CarCtrl::~CarCtrl()
{
    m_timer.stop();

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        delete m_motorGpio[i][0];
        delete m_motorGpio[i][1];
    }
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

    auto& carSpeed = cmn::getSingletonInstance<CarSpeed>();
    carSpeed.setCtrlSteps(motor, 0);
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

void CarCtrl::calculateSpeedCtrl()
{
    int32_t speed, diffSpeed;
    auto& carSpeed = cmn::getSingletonInstance<CarSpeed>();

    for (int i = 0; i < MOTOR_MAX; i++) {
        int ret = carSpeed.getActualSpeed(i, &speed);
        if (!ret) {
            diffSpeed = m_ctrlSpeed[i] - speed;
            if (diffSpeed >= 0) { //speed is low to ctrl
                if (m_ctrlSpeed[i] >= 0)
                    m_currentSpeed[i] += abs(diffSpeed)/2;
                else
                    m_currentSpeed[i] -= abs(diffSpeed)/2;
            } else {    //speed is high to ctrl
                if (m_ctrlSpeed[i] >= 0)
                    m_currentSpeed[i] -= abs(diffSpeed)/2;
                else
                    m_currentSpeed[i] += abs(diffSpeed)/2;
            }
        }
    }
}

void CarCtrl::timerCallback(const asio::error_code &e, void *ctxt)
{
    CarCtrl *pCtrl = static_cast<CarCtrl *>(ctxt);
    auto& carSpeed = cmn::getSingletonInstance<CarSpeed>();
    static int32_t runState[] {0, 0};
    static int32_t runTime[] {0, 0};

    for (int32_t i = 0; i < MOTOR_MAX; i++) {
        if (carSpeed.getCtrlSteps(i) != 0)
            continue;

        runTime[i]++;
        if (runState[i] == 0) {//stop
            pCtrl->setCarState(i, 0);
            if (runTime[i] > (MOTOR_MAX_TIME - abs(pCtrl->m_currentSpeed[i]*MOTOR_SPEED_STEP))) {
                runTime[i] = 0;
                runState[i] = 1;
            }
        } else {  //run
            if (pCtrl->m_ctrlSpeed[i] > 0)
                pCtrl->setCarState(i, 1);
            else if (pCtrl->m_ctrlSpeed[i] < 0)
                pCtrl->setCarState(i, -1);

            if (runTime[i] > abs(pCtrl->m_currentSpeed[i]*MOTOR_SPEED_STEP)) {
                runTime[i] = 0;
                runState[i] = 0;
            }
        }
    }
}

int32_t CarCtrl::setCarState(int32_t motor, int32_t state)
{
    static int32_t direct[] {0, 0};
    if ((state > 0) && (direct[motor] != 1)) {
        m_motorGpio[motor][0]->setValue(0);
        m_motorGpio[motor][1]->setValue(1);
        direct[motor] = 1;
    } else if ((state == 0) && (direct[motor] != 0)) {
        m_motorGpio[motor][0]->setValue(0);
        m_motorGpio[motor][1]->setValue(0);
        direct[motor] = 0;
    } else if ((state < 0) && (direct[motor] != -1)) {
        m_motorGpio[motor][0]->setValue(1);
        m_motorGpio[motor][1]->setValue(0);
        direct[motor] = -1;
    }

    return 0;
}
