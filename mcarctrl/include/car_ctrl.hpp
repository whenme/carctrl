
#ifndef __CAR_CTRL_HPP__
#define __CAR_CTRL_HPP__

#include <ioapi/iotimer.hpp>
#include "gpio.hpp"

#define MOTOR_LEFT    0
#define MOTOR_RIGHT   1
#define MOTOR_MAX     2

#define MOTOR_MAX_SPEED   60
#define MOTOR_MAX_TIME    500
#define MOTOR_SPEED_STEP  (MOTOR_MAX_TIME/MOTOR_MAX_SPEED)


class CarCtrl
{
public:
    CarCtrl(asio::io_service& io_service);
    virtual ~CarCtrl();

    int32_t getCtrlSpeed(int32_t motor, int32_t *speed);
    int32_t setCtrlSpeed(int32_t motor, int32_t speed);

    int32_t setCarState(int32_t car, int32_t state);
    void    calculateSpeedCtrl();

private:
    static void timerCallback(const asio::error_code &e, void *ctxt);

    IoTimer m_timer;
    int32_t m_ctrlSpeed[MOTOR_MAX] {0, 0};
    int32_t m_currentSpeed[MOTOR_MAX] {0, 0};
    Gpio*   m_motorGpio[MOTOR_MAX][2];
};

#endif
