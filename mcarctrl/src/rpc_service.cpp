
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <rpc_service/rpc_service.hpp>
#include "car_ctrl.hpp"

int32_t setCtrlSteps(int32_t motor, int32_t steps)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.setCtrlSteps(motor, steps);
}

int32_t getCtrlSteps(int32_t motor)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getCtrlSteps(motor);
}

int32_t getActualSteps(int32_t motor)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getActualSteps(motor);
}

int32_t setRunTime(int32_t time)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.setRunTime(time);
}

int32_t getActualSpeed(int32_t motor)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getActualSpeed(motor);
}

int32_t setMotorSpeedLevel(int32_t level)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.setMotorSpeedLevel(level);
}

int32_t getMotorSpeedLevel()
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getMotorSpeedLevel();
}

void setMotorPwm(int32_t motor, int32_t pwm)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    ctrl.setMotorPwm(motor, pwm);
}

int32_t getMotorPwm(int32_t motor)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getMotorPwm(motor);
}

void setAllMotorState(int32_t state)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.setAllMotorState(state);
}

int32_t getCtrlMode()
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getCtrlMode();
}

int32_t getMotorNum()
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.getMotorNum();
}

int32_t setCarSteps(CarDirection dir, int32_t steps)
{
    auto& ctrl = cmn::getSingletonInstance<CarCtrl>();
    return ctrl.setCarSteps(dir, steps);
}
