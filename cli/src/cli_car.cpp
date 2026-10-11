// SPDX-License-Identifier: GPL-2.0

#include <cstdlib>

#include <fmt/format.h>
#include <xapi/cmn_singleton.hpp>
#include <cli_car.hpp>
#include <video/sound_intf.hpp>
#include <rpc_client.hpp>

namespace cli
{
CliCar::CliCar(): CliCommandGroup("car")
{
}

void CliCar::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto cliMenu = std::make_unique<Menu>(name);
    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();

    rpc_connect("localhost", rpc_port);

    cliMenu->Insert("show-param",
                    [&](std::ostream& out) {
                        int32_t motorNum = rpc_get_motor_num();
                        int32_t speedLevel = rpc_get_motor_speed_level();
                        out << "speed level: " << speedLevel << "\n";
                        out << "motor number: " << motorNum << "\n";

                        for (int32_t ii = 1; ii <= motorNum; ii++) {
                            int32_t actualSpeed = rpc_get_actual_speed(ii);
                            int32_t actualStep = rpc_get_actual_steps(ii);
                            int32_t motorPwm = rpc_get_motor_pwm(ii);
                            out << fmt::format("motor {} speed={} pwm={} step={}\n", ii, actualSpeed, motorPwm, actualStep);
                        }
                    },
                    "show car speed/pwm/step");

    cliMenu->Insert("set-speedlevel", {"speed: 1-9 for speed level"},
                    [&](std::ostream& out, int32_t speed) {
                        rpc_set_motor_speed_level(speed);
                    },
                    "set motor speed level");

    cliMenu->Insert("set-motorpwm", {"motor:0-all,1~4 motor id", "pwm:0~100"},
                    [&](std::ostream& out, int32_t motor, int32_t pwm) {
                        int32_t motorNum = rpc_get_motor_num();
                        if ((motor < 0) || (motor > motorNum) || (pwm > 100)) {
                            out << "paramet error: motor<0-4>-" << motor << " pwm<0-100>-" << pwm << "\n";
                            return;
                        }

                        if (motor == 0) {
                            for (int32_t ii = 1; ii <= motorNum; ii++) {
                                rpc_set_motor_pwm(ii, pwm);
                            }
                        } else {
                            rpc_set_motor_pwm(motor, pwm);
                        }
                    },
                    "set motor pwm");

    cliMenu->Insert("show-motorstep",
                    [&](std::ostream& out) {
                        int32_t motorNum = rpc_get_motor_num();
                        for (int32_t ii = 1; ii <= motorNum; ii++) {
                            int32_t actualStep = rpc_get_actual_steps(ii);
                            int32_t ctrlStep = rpc_get_ctrl_steps(ii);
                            out << "motor " << ii << " steps: control=" << ctrlStep
                                << " actual=" << actualStep << "\n";
                        }
                    },
                    "show car steps");

    cliMenu->Insert("set-motorstep", {"wheel:0-all,1~4 motor id", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        int32_t motorNum = rpc_get_motor_num();
                        if ((wheel < 0) || (wheel > motorNum) || (steps == 0)) {
                            out << "paramet error: wheel<0-4>-" << wheel << "\n";
                            return;
                        }

                        rpc_set_ctrl_steps(wheel, steps);

                        std::string sound;
                        if (wheel == 0) {
                            if (steps > 0)
                                sound = fmt::format("前进{}步", steps);
                            else
                                sound = fmt::format("后退{}步", abs(steps));
                        } else if (wheel == 1) {
                            if (steps > 0)
                                sound = fmt::format("左前轮前进{}步", steps);
                            else
                                sound = fmt::format("左前轮后退{}步", abs(steps));
                        } else if (wheel == 2) {
                            if (steps > 0)
                                sound = fmt::format("右前轮前进{}步", steps);
                            else
                                sound = fmt::format("右前轮后退{}步", abs(steps));
                        } else if (wheel == 3) {
                            if (steps > 0)
                                sound = fmt::format("左后轮前进{}步", steps);
                            else
                                sound = fmt::format("左后轮后退{}步", abs(steps));
                        } else if (wheel == 4) {
                            if (steps > 0)
                                sound = fmt::format("右后轮前进{}步", steps);
                            else
                                sound = fmt::format("右后轮后退{}步", abs(steps));
                        }
                        soundIntf.speak(sound);
                    },
                    "set motor steps");

    cliMenu->Insert("set-carstep",
                    {"direction:0-forward/backward, 1-left/right, 2-rotation", "step: >0-forward/left, <0-backward/right"},
                    [&](std::ostream& out, int32_t direction, int32_t steps) {
                        std::string sound;
                        int32_t motorNum = rpc_get_motor_num();
                        if (direction == 0) {
                            rpc_set_car_steps(CarDirection::dirUp, steps);
                            if (steps > 0)
                                sound = fmt::format("前进{}步", steps);
                            else
                                sound = fmt::format("后退{}步", steps);
                        } else if (direction == 1) { //left/right
                            rpc_set_car_steps(CarDirection::dirLeft, steps);
                            if (steps > 0)
                                sound = fmt::format("左进{}步", steps);
                            else
                                sound = fmt::format("右进{}步", -steps);
                        } else { //rotation
                            if (motorNum < 4) { // 2 motor not support
                                out << "not support rotation moving with 2 motor" << "\n";
                                sound = fmt::format("两轮车不支持旋转运动");
                            } else {
                                rpc_set_car_steps(CarDirection::dirRotation, steps);
                                if (steps > 0)
                                    sound = fmt::format("顺时针旋转{}步", steps);
                                else
                                    sound = fmt::format("逆时针旋转{}步", -steps);
                            }
                        }
                        soundIntf.speak(sound);
                    },
                    "set car steps");
    cliMenu->Insert("set-runtime", {"time: seconds"},
                    [&](std::ostream& out, int32_t runtime) {
                        rpc_set_run_time(runtime);
                    },
                    "set car run time for speed regulation");
    cliMenu->Insert("set-steer", {"dir: >0 left, =0 stop, <0 right", "time: 0 stop, >0 time"},
                    [&](std::ostream& out, int32_t dir, uint32_t time) {
                        rpc_set_steer_turn(dir, time);
                    },
                    "set steer direction and time. dir: >0 left, =0 stop, <0 right. time: 0 stop, >0 time");
    cliMenu->Insert("stop-motor",
                    [&](std::ostream& out) {
                        rpc_set_all_motor_state(0);
                    },
                    "stop all motors");

    if (rootMenu != nullptr) {
        rootMenu->Insert(std::move(cliMenu));
    }
}

void CliCar::quitApplications()
{
    rpc_quit_app(0);
    std::exit(0);
}

}  // namespace cli
