// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <ioapi/iotimer.hpp>
#include "gpio.hpp"
#include "car_speed.hpp"

#define MOTOR_MAX_TIME    500
#define MOTOR_SPEED_STEP  (MOTOR_MAX_TIME/MOTOR_MAX_SPEED)

#define MOTOR_MAX_SUBSTEP 20

#define CTRL_MODE_STEP    0
#define CTRL_MODE_SPEED   1
#define CTRL_MODE_TIME    2

class CarCtrl
{
public:
    CarCtrl(asio::io_service& io_service);
    virtual ~CarCtrl();

    int32_t getActualSpeed(int32_t motor);
    int32_t setCtrlSteps(int32_t motor, int32_t steps);
    int32_t getCtrlSteps(int32_t motor);
    int32_t getActualSteps(int32_t motor);
    int32_t setRunTime(int32_t time);
    int32_t setMotorSpeedLevel(int32_t level);
    int32_t getMotorSpeedLevel();
    void    setAllMotorState(int32_t state);

    int32_t getCtrlMode();
    int32_t getMotorNum();

    void    setMotorPwm(int32_t motor, int32_t pwm);
    int32_t getMotorPwm(int32_t motor);

private:
    static void runTimeCallback(const asio::error_code &e, void *ctxt);

    CarSpeed m_carSpeed;
    IoTimer  m_runTimer;
    int32_t  m_ctrlMode {CTRL_MODE_STEP};
    bool     m_straight { false };
};
