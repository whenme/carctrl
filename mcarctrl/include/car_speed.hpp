// SPDX-License-Identifier: GPL-2.0
#ifndef __CAR_SPEED_HPP__
#define __CAR_SPEED_HPP__

#include <ioapi/iotimer.hpp>
#include "car_ctrl.hpp"
#include "gpio.hpp"

class CarSpeed
{
public:
    CarSpeed(asio::io_service& io_service);
    virtual ~CarSpeed();

    int32_t getActualSpeed(int32_t motor, int32_t *speed);

    int32_t setCtrlSteps(int32_t motor, int32_t steps);
    int32_t getCtrlSteps(int32_t motor);
    int32_t getActualSteps(int32_t motor);

private:
    static void *carSpeedThread(void *args);
    static void timerSpeedCallback(const asio::error_code &e, void *ctxt);

    asio::io_service& m_ioService;
    IoTimer m_timerSpeed;
    pthread_t m_tipd;
    int32_t m_ctrlSetSteps[MOTOR_MAX] {0, 0};
    int32_t m_ctrlSteps[MOTOR_MAX] {0, 0};
    int32_t m_actualSteps[MOTOR_MAX] {0, 0};
    int32_t m_actualSpeed[MOTOR_MAX]{0, 0};
    int32_t m_swCounter[MOTOR_MAX]{0, 0};
    Gpio *m_gpioSpeed[MOTOR_MAX];
};

#endif
