// SPDX-License-Identifier: GPL-2.0
#ifndef __CAR_SPEED_HPP__
#define __CAR_SPEED_HPP__

#include <ioapi/iotimer.hpp>
#include <ioapi/iothread.hpp>
#include "motor.hpp"

class CarCtrl;

class CarSpeed
{
public:
    CarSpeed(asio::io_service& io_service, CarCtrl *carCtrl);
    virtual ~CarSpeed();

    int32_t getActualSpeed(int32_t motor);
    int32_t getCurrentCtrlSpeed(int32_t motor);

    void    setRunSteps(int32_t motor, int32_t steps);
    bool    getRunState(int32_t motor);
    int32_t getActualSteps(int32_t motor);
    void    setMotorState(int32_t motor, int32_t state);
    int32_t getMotorState(int32_t motor);

private:
    static void threadFun(void *ctxt);
    static void timerSpeedCallback(const asio::error_code &e, void *ctxt);
    void        calculateSpeedCtrl();

    asio::io_service& m_ioService;
    IoTimer  m_timerSpeed;
    IoThread m_speedThread;
    CarCtrl* m_carCtrl;

    int32_t m_ctrlSetSteps[MOTOR_MAX]{0, 0};
    int32_t m_ctrlSteps[MOTOR_MAX] {0, 0};
    int32_t m_actualSteps[MOTOR_MAX] {0, 0};

    int32_t m_actualSpeed[MOTOR_MAX] {0, 0};
    int32_t m_currentCtrlSpeed[MOTOR_MAX] {0, 0};
    int32_t m_swCounter[MOTOR_MAX] {0, 0};
    bool    m_runState[MOTOR_MAX] {false, false};
    Gpio*   m_gpioSpeed[MOTOR_MAX];
    Motor*  m_motor[MOTOR_MAX];

    static constexpr int32_t m_maxPwm[MOTOR_MAX] {100, 100};
    int32_t m_ctrlPwm[MOTOR_MAX] {40, 20};
};

#endif
