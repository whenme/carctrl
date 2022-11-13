// SPDX-License-Identifier: GPL-2.0
#ifndef __CAR_CTRL_HPP__
#define __CAR_CTRL_HPP__

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

    int32_t getCtrlMode();
    int32_t getMotorNum();

    void    setMotorPwm(int32_t motor, int32_t pwm);
    int32_t getMotorPwm(int32_t motor);

private:
    static void timerCallback(const asio::error_code &e, void *ctxt);
    static void runTimeCallback(const asio::error_code &e, void *ctxt);
    void checkNextSteps();
    void checkNextTime();

    CarSpeed m_carSpeed;
    IoTimer m_timer;
    IoTimer m_runTimer;
    int32_t m_ctrlMode {CTRL_MODE_STEP};
    bool    m_straight { false };
    bool    m_runState[MOTOR_NUM_MAX] {false, false, false, false}; // in run(true) or stop(false)
    int32_t m_ctrlSetSteps[MOTOR_NUM_MAX]{0, 0};
    int32_t m_ctrlSteps[MOTOR_NUM_MAX]   {0, 0};
    int32_t m_ctrlSubSteps[MOTOR_NUM_MAX]{0, 0};
    int32_t m_actualSteps[MOTOR_NUM_MAX] {0, 0};

    int32_t m_ctrlSpeed[MOTOR_NUM_MAX]   {0, 0};
    int32_t m_currentSpeed[MOTOR_NUM_MAX]{0, 0};
};

#endif
