#include <xapi/cmn_singleton.hpp>
#include <xapi/easylog.hpp>
#include <rpc_service.hpp>
#include "car_ctrl.hpp"

namespace {
CarCtrl& getCarCtrl()
{
    static auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();
    return carCtrl;
}
}  // namespace

int32_t setCtrlSteps(int32_t motor, int32_t steps)
{
    return getCarCtrl().setCtrlSteps(motor, steps);
}

int32_t getCtrlSteps(int32_t motor)
{
    return getCarCtrl().getCtrlSteps(motor);
}

int32_t getActualSteps(int32_t motor)
{
    return getCarCtrl().getActualSteps(motor);
}

int32_t setRunTime(int32_t time)
{
    return getCarCtrl().setRunTime(time);
}

int32_t getActualSpeed(int32_t motor)
{
    return getCarCtrl().getActualSpeed(motor);
}

int32_t setMotorSpeedLevel(int32_t level)
{
    return getCarCtrl().setMotorSpeedLevel(level);
}

int32_t getMotorSpeedLevel()
{
    return getCarCtrl().getMotorSpeedLevel();
}

void setMotorPwm(int32_t motor, int32_t pwm)
{
    getCarCtrl().setMotorPwm(motor, pwm);
}

int32_t getMotorPwm(int32_t motor)
{
    return getCarCtrl().getMotorPwm(motor);
}

void setAllMotorState(int32_t state)
{
    return getCarCtrl().setAllMotorState(state);
}

int32_t getCtrlMode()
{
    return getCarCtrl().getCtrlMode();
}

int32_t getMotorNum()
{
    return getCarCtrl().getMotorNum();
}

int32_t setCarSteps(CarDirection dir, int32_t steps)
{
    return getCarCtrl().setCarSteps(dir, steps);
}

int32_t setCarMoving(CarDirection dir)
{
    return getCarCtrl().setCarMoving(dir);
}

int32_t setSteerTurn(int32_t dir, uint32_t time)
{
    return getCarCtrl().steerTurn(dir, time);
}

void quitApp(int32_t param)
{
    exit(param);
}
