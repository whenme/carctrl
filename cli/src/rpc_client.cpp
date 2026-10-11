// SPDX-License-Identifier: GPL-2.0

#include <rpc_client.hpp>

#include <string>
#include <utility>

#include <xapi/easylog.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

using namespace async_simple::coro;

namespace
{

coro_rpc::coro_rpc_client& rpcClient()
{
    static coro_rpc::coro_rpc_client client;
    return client;
}

template<auto func, typename... Args>
int32_t callInt(Args&&... args)
{
    auto ret = syncAwait(rpcClient().call<func>(std::forward<Args>(args)...));
    if (!ret) {
        ctrllog::error("failed to call function...");
        return 0;
    }
    return ret.value();
}

template<auto func, typename... Args>
void callVoid(Args&&... args)
{
    auto ret = syncAwait(rpcClient().call<func>(std::forward<Args>(args)...));
    if (!ret) {
        ctrllog::error("failed to call function...");
    }
}

}  // namespace

void rpc_connect(const std::string& host, uint16_t port)
{
    syncAwait(rpcClient().connect(host, std::to_string(port)));
}

int32_t rpc_get_ctrl_steps(int32_t motor)
{
    return callInt<getCtrlSteps>(motor);
}

int32_t rpc_set_ctrl_steps(int32_t motor, int32_t steps)
{
    return callInt<setCtrlSteps>(motor, steps);
}

int32_t rpc_set_car_steps(CarDirection dir, int32_t steps)
{
    return callInt<setCarSteps>(dir, steps);
}

int32_t rpc_set_car_moving(CarDirection dir)
{
    return callInt<setCarMoving>(dir);
}

int32_t rpc_get_actual_steps(int32_t motor)
{
    return callInt<getActualSteps>(motor);
}

int32_t rpc_set_run_time(int32_t time)
{
    return callInt<setRunTime>(time);
}

int32_t rpc_get_actual_speed(int32_t motor)
{
    return callInt<getActualSpeed>(motor);
}

int32_t rpc_set_motor_speed_level(int32_t level)
{
    return callInt<setMotorSpeedLevel>(level);
}

int32_t rpc_get_motor_speed_level()
{
    return callInt<getMotorSpeedLevel>();
}

void rpc_set_motor_pwm(int32_t motor, int32_t pwm)
{
    callVoid<setMotorPwm>(motor, pwm);
}

int32_t rpc_get_motor_pwm(int32_t motor)
{
    return callInt<getMotorPwm>(motor);
}

void rpc_set_all_motor_state(int32_t state)
{
    callVoid<setAllMotorState>(state);
}

int32_t rpc_get_ctrl_mode()
{
    return callInt<getCtrlMode>();
}

int32_t rpc_get_motor_num()
{
    return callInt<getMotorNum>();
}

int32_t rpc_set_steer_turn(int32_t dir, uint32_t time)
{
    return callInt<setSteerTurn>(dir, time);
}

void rpc_quit_app(int32_t param)
{
    callVoid<quitApp>(param);
}
