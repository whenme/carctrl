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

    int32_t getInputGpioFd();

    void    setRunState(int32_t state);
    int32_t getRunState();

    void     setCtrlSteps(int32_t steps);
    int32_t& getCtrlSteps();

    void     setActualSteps(int32_t steps);
    int32_t& getActualSteps();

    void    setRunPwm(int32_t pwm);
    int32_t getRunPwm();
    int32_t getStopPwm();
    static int32_t getMaxPwm();

    int32_t m_actualSpeed { 0 };
    int32_t m_swCounter { 0 };
private:
    Gpio*   m_outputGpio[2];
    Gpio*   m_inputGpio;
    int32_t m_portState[2];
    int32_t m_runState {MOTOR_STATE_STOP};
    int32_t m_ctrlSteps { 0 };
	int32_t m_actualSteps { 0 };

    static constexpr int32_t m_maxPwm { 100 };
    int32_t m_ctrlPwm { 50 };
};

#endif