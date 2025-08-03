// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <vector>
#include <xapi/iotimer.hpp>
#include <xapi/cmn_thread.hpp>
#include "motor.hpp"
#include "steer.hpp"

static constexpr std::string_view k_deviceNamePc = "orangepipc";
static constexpr std::string_view k_deviceNameM1 = "nanopim1";
static constexpr std::string_view k_deviceNameOneplus = "orangepioneplus";

class CarCtrl;

class CarSpeed
{
public:
    CarSpeed(asio::io_context& context, CarCtrl *carCtrl);
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

    // time >0 turn left, <0 turn right, =0 stop. 
    void    steerTurn(int32_t time = 0);

private:
    static void threadFun(void *ctxt);
    void        initJsonParam();
    void        motorPwmCtrl();

    asio::io_context& m_context;
    cmn::CmnThread    m_speedThread;
    CarCtrl* m_carCtrl;
    Steer*   m_steer {nullptr};
    int32_t  m_motorNum { 0 };
    int32_t  m_speedLevel { 0 };
    std::vector<Motor*> m_motor;
    std::vector<std::vector<int32_t>> m_pwmVect;
};
