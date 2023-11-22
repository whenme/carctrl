// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <ylt/coro_rpc/coro_rpc_context.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

enum class CarDirection {
    dirInvalid,
    dirUpDown,
    dirLeftRight,
    dirRotation
};

int32_t getCtrlSteps(int32_t motor);
int32_t setCtrlSteps(int32_t motor, int32_t steps);

int32_t setCarSteps(CarDirection dir, int32_t steps);

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

using namespace async_simple::coro;

template<auto func>
int32_t rpc_call_int_param(coro_rpc::coro_rpc_client& client)
{
    auto ret = syncAwait(client.call<func>());
    if (!ret) {
        ctrllog::error("failed to call function...");
        return 0;
    } else {
        return ret.value();
    }
}

template<auto func, typename... Args>
int32_t rpc_call_int_param(coro_rpc::coro_rpc_client& client, Args... args)
{
    auto ret = syncAwait(client.call<func>(std::forward<Args>(args)...));
    if (!ret) {
        ctrllog::error("failed to call function...");
        return 0;
    } else {
        return ret.value();
    }
}

template<auto func, typename... Args>
void rpc_call_void_param(coro_rpc::coro_rpc_client& client, Args... args)
{
    auto ret = syncAwait(client.call<func>(std::forward<Args>(args)...));
    if (!ret) {
        ctrllog::error("failed to call function...");
    }
}