// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>

static constexpr uint16_t rpc_port = 8802;

enum class CarDirection {
    dirInvalid,
    dirUp,
    dirDown,
    dirLeft,
    dirRight,
    dirRotation
};

int32_t getCtrlSteps(int32_t motor);
int32_t setCtrlSteps(int32_t motor, int32_t steps);

int32_t setCarSteps(CarDirection dir, int32_t steps);
int32_t setCarMoving(CarDirection dir);

int32_t getActualSteps(int32_t motor);
int32_t setRunTime(int32_t time);

int32_t getActualSpeed(int32_t motor);
int32_t setMotorSpeedLevel(int32_t level);
int32_t getMotorSpeedLevel();

void    setMotorPwm(int32_t motor, int32_t pwm);
int32_t getMotorPwm(int32_t motor);

void    setAllMotorState(int32_t state);
int32_t getCtrlMode();
int32_t getMotorNum();
int32_t setSteerTurn(int32_t dir, uint32_t time);

void quitApp(int32_t param);
