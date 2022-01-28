
#ifndef __CAR_CTRL_HPP__
#define __CAR_CTRL_HPP__

#include <stdint.h>
#include <pthread.h>
#include "Gpio.hpp"

#define MOTOR_LEFT    0
#define MOTOR_RIGHT   1
#define MOTOR_MAX     2

#define MOTOR_MAX_SPEED   60
#define MOTOR_MAX_TIME    1000
#define MOTOR_SPEED_STEP  (MOTOR_MAX_TIME/MOTOR_MAX_SPEED)

#define SPEED_CTRL_TIME    3

class CarCtrl
{
public:
    static CarCtrl *getInstance();
    int32_t getCtrlSpeed(int32_t motor, int32_t *speed);
    int32_t setCtrlSpeed(int32_t motor, int32_t speed);
    void    calculateSpeedCtrl();

private:
    CarCtrl();
    virtual ~CarCtrl();

    void initApp();
    int32_t setCarState(int32_t car, int32_t state);
    static void *carCtrlPthread(void *args);

private:
    static CarCtrl *m_pInstance;
    int32_t m_ctrlSpeed[MOTOR_MAX];
    int32_t m_currentSpeed[MOTOR_MAX];
    pthread_t m_tipd;
    Gpio *m_motorGpio[MOTOR_MAX][2];
};

#endif
