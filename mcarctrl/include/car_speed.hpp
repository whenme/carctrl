// SPDX-License-Identifier: GPL-2.0
#ifndef __CAR_SPEED_HPP__
#define __CAR_SPEED_HPP__

#include <ioapi/iotimer.hpp>
#include "gpio.hpp"

#define MOTOR_LEFT    0
#define MOTOR_RIGHT   1
#define MOTOR_MAX     2

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

private:
    static void *carSpeedThread(void *args);
    static void timerSpeedCallback(const asio::error_code &e, void *ctxt);
    void calculateSpeedCtrl();

    asio::io_service& m_ioService;
    IoTimer   m_timerSpeed;
    CarCtrl*  m_carCtrl;
    pthread_t m_tipd;

    int32_t m_ctrlSetSteps[MOTOR_MAX]{0, 0};
    int32_t m_ctrlSteps[MOTOR_MAX] {0, 0};
    int32_t m_actualSteps[MOTOR_MAX] {0, 0};

    int32_t m_actualSpeed[MOTOR_MAX] {0, 0};
    int32_t m_currentCtrlSpeed[MOTOR_MAX] {0, 0};
    int32_t m_swCounter[MOTOR_MAX] {0, 0};
    bool    m_runState[MOTOR_MAX] {false, false};
    Gpio*   m_gpioSpeed[MOTOR_MAX];
};

#endif
