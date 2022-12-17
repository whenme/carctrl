// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <vector>
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
    int32_t getMotorNum();
    void    setMotorSpeedLevel(int32_t level);
    int32_t getMotorSpeedLevel();

    void    setRunSteps(int32_t motor, int32_t steps);
    bool    getRunState(int32_t motor);

    void    setActualSteps(int32_t motor, int32_t steps);
    int32_t getActualSteps(int32_t motor);
    int32_t getCtrlSteps(int32_t motor);

    void    setMotorState(int32_t motor, int32_t state);
    int32_t getMotorState(int32_t motor);

    void    setMotorPwm(int32_t motor, int32_t pwm);
    int32_t getMotorPwm(int32_t motor);

private:
    static void threadFun(void *ctxt);
    static void timerSpeedCallback(const asio::error_code &e, void *ctxt);
    void        initJsonParam();
    void        motorPwmCtrl();

    asio::io_service& m_ioService;
    IoTimer  m_timerSpeed;
    IoThread m_speedThread;
    CarCtrl* m_carCtrl;
    int32_t  m_motorNum { 0 };
    int32_t  m_speedLevel { 0 };
    std::vector<Motor*> m_motor;
    std::vector<std::vector<int32_t>> m_pwmVect;
};
