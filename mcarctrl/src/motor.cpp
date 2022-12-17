// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <spdlog/easylog.hpp>
#include "motor.hpp"

Motor::Motor(std::vector<uint32_t> port)
{
    if (port.size() != 3) {
        easylog::error("gpio number error {}", port.size());
        return;
    }

    m_outputGpio[0] = new Gpio(port.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_outputGpio[1] = new Gpio(port.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_inputGpio = new Gpio(port.at(2), GPIO_DIR_IN, GPIO_EDGE_RISING);

    if ((m_outputGpio[0] == nullptr) || (m_outputGpio[1] == nullptr) || (m_inputGpio == nullptr)) {
        easylog::warn("fail to create motor from output gpio {},{} input gpio {}", port.at(0), port.at(1), port.at(2));
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

void Motor::setCtrlSteps(int32_t steps)
{
    m_ctrlSteps = steps;
    setRunState(steps);
}

void Motor::setRunState(int32_t state)
{
    if (state > 0)
        m_runState = MOTOR_STATE_FORWARD;
    else if (!state)
        m_runState = MOTOR_STATE_STOP;
    else
        m_runState = MOTOR_STATE_BACK;
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
