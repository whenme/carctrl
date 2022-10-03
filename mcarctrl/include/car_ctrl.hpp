// SPDX-License-Identifier: GPL-2.0
#ifndef __CAR_CTRL_HPP__
#define __CAR_CTRL_HPP__

#include <ioapi/iotimer.hpp>
#include "gpio.hpp"
#include "car_speed.hpp"

#define MOTOR_MAX_SPEED   60
#define MOTOR_MAX_TIME    500
#define MOTOR_SPEED_STEP  (MOTOR_MAX_TIME/MOTOR_MAX_SPEED)

#define MOTOR_MAX_SUBSTEP 20

class CarCtrl
{
public:
    CarCtrl(asio::io_service& io_service);
    virtual ~CarCtrl();

    int32_t getActualSpeed(int32_t motor);
    int32_t getCtrlSpeed(int32_t motor, int32_t *speed);
    int32_t setCtrlSpeed(int32_t motor, int32_t speed);

    int32_t setCtrlSteps(int32_t motor, int32_t steps);
    int32_t getCtrlSteps(int32_t motor);
    int32_t getActualSteps(int32_t motor);

    void    calculateSpeedCtrl();
    bool    getCtrlState();

private:
    static void timerCallback(const asio::error_code &e, void *ctxt);
    void checkNextSteps();

    CarSpeed m_carSpeed;
    IoTimer m_timer;
    bool    m_stepSpeed { false }; //run for step(false) or speed(true)
    bool    m_straight { false };
    bool    m_stepState[MOTOR_MAX] {false, false}; // in run(true) or stop(false)
    int32_t m_ctrlSetSteps[MOTOR_MAX]{0, 0};
    int32_t m_ctrlSteps[MOTOR_MAX]   {0, 0};
    int32_t m_ctrlSubSteps[MOTOR_MAX]{0, 0};
    int32_t m_actualSteps[MOTOR_MAX] {0, 0};

    int32_t m_ctrlSpeed[MOTOR_MAX]   {0, 0};
    int32_t m_currentSpeed[MOTOR_MAX]{0, 0};
};

#endif
