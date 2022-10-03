// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include "motor.hpp"

Motor::Motor(std::vector<uint32_t> port) :
    m_runState(MOTOR_STATE_STOP)
{
    m_motorGpio[0] = new Gpio(port.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_motorGpio[1] = new Gpio(port.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    if ((m_motorGpio[0] == nullptr) || (m_motorGpio[1] == nullptr)) {
        std::cout <<"Motor: fail to create motor from gpio " << port.at(0) << " and " << port.at(1) << std::endl;
    }

    setRunState(MOTOR_STATE_STOP);
}

Motor::~Motor()
{
    setRunState(MOTOR_STATE_STOP);

    delete m_motorGpio[0];
    delete m_motorGpio[1];
}

int32_t Motor::getRunState()
{
    return m_runState;
}

void Motor::setRunState(int32_t state)
{
    auto setPortState = [&](int32_t port, int32_t state) {
        if (m_portState[port] != state) {
            m_portState[port] = state;
            m_motorGpio[port]->setValue(state);
        }
    };

    if (m_runState == state)
        return;

    if (state > 0) {
        setPortState(0, 0);
        setPortState(1, 1);
    } else if (state == 0) {
        setPortState(0, 0);
        setPortState(1, 0);
    } else {
        setPortState(0, 1);
        setPortState(1, 0);
    }

    m_runState = state;
}
