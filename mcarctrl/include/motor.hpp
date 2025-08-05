// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <vector>
#include "gpio.hpp"

#define MOTOR_FRONT_LEFT    0
#define MOTOR_FRONT_RIGHT   1
#define MOTOR_BACK_LEFT     2
#define MOTOR_BACK_RIGHT    3
#define MOTOR_NUM_MAX       4

enum class MotorState : int32_t {
    Forward = 1,
    Stop = 0,
    Back = -1
};

class Motor
{
public:
    Motor(std::vector<uint32_t> port);
    virtual ~Motor();

    int32_t getInputGpioFd();

    void setRunState(MotorState state);
    inline MotorState getRunState()    { return m_runState; }

    void setNowState(MotorState state);
    inline MotorState getNowState()    { return m_nowState; }

    void setCtrlSteps(int32_t steps);
    inline int32_t& getCtrlSteps()  { return m_ctrlSteps; }

    inline void moveActualSteps(int32_t steps) { m_actualSteps += steps; }
    inline void setActualSteps(int32_t steps)  { m_actualSteps = steps; }
    inline int32_t getActualSteps()            { return m_actualSteps; }

    inline void setRunPwm(int32_t pwm) { m_ctrlPwm = pwm; }
    inline int32_t getRunPwm()         { return m_ctrlPwm; }
    inline int32_t getStopPwm()        { return m_maxPwm - m_ctrlPwm; }
    static int32_t getMaxPwm()         { return m_maxPwm; }

    int32_t m_actualSpeed { 0 };
    int32_t m_swCounter { 0 };

private:
    Gpio*   m_outputGpio[2] {nullptr, nullptr};
    Gpio*   m_inputGpio {nullptr};
    int32_t m_portState[2];
    MotorState m_runState { MotorState::Stop };
    MotorState m_nowState { MotorState::Stop };
    int32_t m_ctrlSteps { 0 };
    int32_t m_actualSteps { 0 };

    static constexpr int32_t m_maxPwm { 100 };
    int32_t m_ctrlPwm { 50 };
};
