// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <xapi/easylog.hpp>
#include "motor.hpp"

Motor::Motor(std::vector<uint32_t> port)
{
    m_outputGpio[0] = new Gpio(port.at(0), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    m_outputGpio[1] = new Gpio(port.at(1), GPIO_DIR_OUT, GPIO_EDGE_NONE);
    if (port.size() > 2) {
        m_inputGpio = new Gpio(port.at(2), GPIO_DIR_IN, GPIO_EDGE_RISING);
    }

    if ((m_outputGpio[0] == nullptr) || (m_outputGpio[1] == nullptr)) {
        ctrllog::warn("fail to create motor from output gpio {},{}", port.at(0), port.at(1));
    }

    setNowState(MotorState::Stop);
}

Motor::~Motor()
{
    setNowState(MotorState::Stop);

    delete m_outputGpio[0];
    delete m_outputGpio[1];
    if (m_inputGpio) {
        delete m_inputGpio;
    }
}

void Motor::setCtrlSteps(int32_t steps)
{
    m_ctrlSteps = steps;

    MotorState state;
    if (steps > 0)
        state = MotorState::Forward;
    else if(steps == 0)
        state = MotorState::Stop;
    else
        state = MotorState::Back;

    setRunState(state);
}

void Motor::setRunState(MotorState state)
{
    m_runState = state;
}

void Motor::setNowState(MotorState state)
{
    auto setPortState = [&](int32_t port, int32_t stat) {
        if (m_portState[port] != stat) {
            m_portState[port] = stat;
            m_outputGpio[port]->setValue(stat);
        }
    };

    if (m_nowState == state)
        return;

    if (state == MotorState::Forward) {
        setPortState(0, 0);
        setPortState(1, 1);
    } else if (state == MotorState::Stop) {
        setPortState(0, 0);
        setPortState(1, 0);
    } else {
        setPortState(0, 1);
        setPortState(1, 0);
    }

    m_nowState = state;
}

int32_t Motor::getInputGpioFd()
{
    if (m_inputGpio) {
        return m_inputGpio->getGpioFd();
    } else {
        return 0;
    }
}
