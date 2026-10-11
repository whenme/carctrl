// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
#include <string>
#include <rpc_service.hpp>

// Plain wrappers around the coro_rpc client. The template headers stay in
// rpc_client.cpp so other translation units can compile on small armhf boards.

void rpc_connect(const std::string& host, uint16_t port);

int32_t rpc_get_ctrl_steps(int32_t motor);
int32_t rpc_set_ctrl_steps(int32_t motor, int32_t steps);
int32_t rpc_set_car_steps(CarDirection dir, int32_t steps);
int32_t rpc_set_car_moving(CarDirection dir);
int32_t rpc_get_actual_steps(int32_t motor);
int32_t rpc_set_run_time(int32_t time);
int32_t rpc_get_actual_speed(int32_t motor);
int32_t rpc_set_motor_speed_level(int32_t level);
int32_t rpc_get_motor_speed_level();
void    rpc_set_motor_pwm(int32_t motor, int32_t pwm);
int32_t rpc_get_motor_pwm(int32_t motor);
void    rpc_set_all_motor_state(int32_t state);
int32_t rpc_get_ctrl_mode();
int32_t rpc_get_motor_num();
int32_t rpc_set_steer_turn(int32_t dir, uint32_t time);
void    rpc_quit_app(int32_t param);
