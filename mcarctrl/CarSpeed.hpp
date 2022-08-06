
#ifndef __CAR_SPEED_HPP__
#define __CAR_SPEED_HPP__

#include "CarCtrl.hpp"
#include "Gpio.hpp"


class CarSpeed
{
public:
    static CarSpeed *getInstance();
    int32_t getActualSpeed(int32_t motor, int32_t *speed);

private:
    CarSpeed();
    virtual ~CarSpeed();
    void initApp();
    void initSpeedTimer();
    static void *carSpeedThread(void *args);
    static void timerCallback(int32_t dat);

    static CarSpeed *m_pInstance;
    pthread_t m_tipd;
    int32_t m_actualSpeed[MOTOR_MAX];
    int32_t m_swCounter[MOTOR_MAX];
    Gpio *m_gpioSpeed[MOTOR_MAX];
};

#endif
