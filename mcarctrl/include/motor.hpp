// SPDX-License-Identifier: GPL-2.0
#ifndef __MOTOR_HPP__
#define __MOTOR_HPP__

#include <vector>
#include "gpio.hpp"

#define MOTOR_FRONT_LEFT    0
#define MOTOR_FRONT_RIGHT   1
#define MOTOR_BACK_LEFT     2
#define MOTOR_BACK_RIGHT    3
#define MOTOR_NUM_MAX       4

#define MOTOR_STATE_FORWARD    1
#define MOTOR_STATE_BACK      -1
#define MOTOR_STATE_STOP       0

class Motor
{
public:
    Motor(std::vector<uint32_t> port);
    virtual ~Motor();

    inline int32_t getInputGpioFd() { return m_inputGpio->getGpioFd(); }

    inline void setRunState(int32_t state) { m_runState = state; }
    inline int32_t getRunState()           { return m_runState; }

    void setNowState(int32_t state);
    inline int32_t getNowState()           { return m_nowState; }

    inline void setCtrlSteps(int32_t steps) { m_ctrlSteps = steps; }
    inline int32_t& getCtrlSteps()          { return m_ctrlSteps; }

    inline void setActualSteps(int32_t steps) { m_actualSteps = steps; }
    inline int32_t getActualSteps()           { return m_actualSteps; }

    inline void setRunPwm(int32_t pwm) { m_ctrlPwm = pwm; }
    inline int32_t getRunPwm()         { return m_ctrlPwm; }
    inline int32_t getStopPwm()        { return m_maxPwm - m_ctrlPwm; }
    static int32_t getMaxPwm()         { return m_maxPwm; }

    int32_t m_actualSpeed { 0 };
    int32_t m_swCounter { 0 };

private:
    Gpio*   m_outputGpio[2];
    Gpio*   m_inputGpio;
    int32_t m_portState[2];
    int32_t m_runState { MOTOR_STATE_STOP };
    int32_t m_nowState { MOTOR_STATE_FORWARD };
    int32_t m_ctrlSteps { 0 };
    int32_t m_actualSteps { 0 };

    static constexpr int32_t m_maxPwm { 100 };
    int32_t m_ctrlPwm { 50 };
};

#endif