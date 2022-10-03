// SPDX-License-Identifier: GPL-2.0
#ifndef __MOTOR_HPP__
#define __MOTOR_HPP__

#include <vector>
#include "gpio.hpp"

#define MOTOR_LEFT    0
#define MOTOR_RIGHT   1
#define MOTOR_MAX     2

#define MOTOR_STATE_FORWARD    1
#define MOTOR_STATE_BACK      -1
#define MOTOR_STATE_STOP       0

class Motor
{
public:
    Motor(std::vector<uint32_t> port);
    virtual ~Motor();

    void    setRunState(int32_t state);
    int32_t getRunState();

private:
    Gpio*   m_motorGpio[2];
    int32_t m_runState;
    int32_t m_portState[2];
};

#endif