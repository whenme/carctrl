// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include "motor.hpp"

Motor::Motor(std::vector<uint32_t> port)
{
    if (port.size() != 3) {
        std::cout << "Motor: GPIO number error..." << std::endl;
        return;
    }

    m_outputGpio[0] = new Gpio(port.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_outputGpio[1] = new Gpio(port.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_inputGpio = new Gpio(port.at(2), GPIO_DIR_IN, GPIO_EDGE_RISING);

    if ((m_outputGpio[0] == nullptr) || (m_outputGpio[1] == nullptr) || (m_inputGpio == nullptr)) {
        std::cout <<"Motor: fail to create motor from output gpio " << port.at(0) << "," << port.at(1)
                  << " input gpio " << port.at(2) << std::endl;
    }

    setNowState(MOTOR_STATE_STOP);
}

Motor::~Motor()
{
    setNowState(MOTOR_STATE_STOP);

    delete m_outputGpio[0];
    delete m_outputGpio[1];
    delete m_inputGpio;
}

void Motor::setNowState(int32_t state)
{
    auto setPortState = [&](int32_t port, int32_t state) {
        if (m_portState[port] != state) {
            m_portState[port] = state;
            m_outputGpio[port]->setValue(state);
        }
    };

    if (m_nowState == state)
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

    m_nowState = state;
}