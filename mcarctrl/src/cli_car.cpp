// SPDX-License-Identifier: GPL-2.0

#include <ioapi/cmn_singleton.hpp>
#include <cli/cli_impl.h>
#include <video/sound_intf.hpp>
#include <car_ctrl.hpp>
#include <cli_car.hpp>

namespace cli
{
void CliCar::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto cliMenu = std::make_unique<Menu>(name);
    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

    cliMenu->insert("show-param", showParam, "show car speed/pwm/step");
    cliMenu->insert("set-speedlevel", {"speed: 1-9 for speed level"},
                    [&](std::ostream& out, int32_t speed) {
                        carCtrl.setMotorSpeedLevel(speed);
                    },
                    "set motor speed level");

    cliMenu->insert("set-motorpwm", {"motor:0-all,1~4 motor id", "pwm:0~100"},
                    [&](std::ostream& out, int32_t motor, int32_t pwm) {
                        if ((motor < 0) || (motor > carCtrl.getMotorNum()) || (pwm > 100)) {
                            out << "paramet error: motor<0-4>-" << motor << " pwm<0-100>-" << pwm << "\n";
                            return;
                        }

                        if (motor == 0) {
                            for (int32_t i = 0; i < carCtrl.getMotorNum(); i++) {
                                carCtrl.setMotorPwm(i, pwm);
                            }
                        } else {
                            carCtrl.setMotorPwm(motor - 1, pwm);
                        }
                    },
                    "set motor pwm");

    cliMenu->insert("show-motorstep", showSteps, "show car steps");
    cliMenu->insert("set-motorstep", {"wheel:0-all,1~4 motor id", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        if ((wheel < 0) || (wheel > carCtrl.getMotorNum()) || (steps == 0)) {
                            out << "paramet error: wheel<0-4>-" << wheel << "\n";
                            return;
                        }

                        char sound[128] = {0};
                        if (wheel == 0) {
                            carCtrl.setCtrlSteps(0, steps);
                            if (steps > 0)
                                sprintf(sound, "前进%d步", steps);
                            else
                                sprintf(sound, "后退%d步", abs(steps));
                        } else if (wheel == 1) {
                            carCtrl.setCtrlSteps(wheel, steps);
                            if (steps > 0)
                                sprintf(sound, "左前轮前进%d步", steps);
                            else
                                sprintf(sound, "左前轮后退%d步", abs(steps));
                        } else if (wheel == 2) {
                            carCtrl.setCtrlSteps(wheel, steps);
                            if (steps > 0)
                                sprintf(sound, "右前轮前进%d步", steps);
                            else
                                sprintf(sound, "右前轮后退%d步", abs(steps));
                        } else if (wheel == 3) {
                            carCtrl.setCtrlSteps(wheel, steps);
                            if (steps > 0)
                                sprintf(sound, "左后轮前进%d步", steps);
                            else
                                sprintf(sound, "左后轮后退%d步", abs(steps));
                        } else if (wheel == 4) {
                            carCtrl.setCtrlSteps(wheel, steps);
                            if (steps > 0)
                                sprintf(sound, "右后轮前进%d步", steps);
                            else
                                sprintf(sound, "右后轮后退%d步", abs(steps));
                        }
                        soundIntf.speak(sound);
                    },
                    "set motor steps");

    cliMenu->insert("set-carstep",
                    {"direction:0-forward/backward, 1-left/right, 2-rotation", "steps: >0-forward or left, <0-backward or right"},
                    [&](std::ostream& out, int32_t direction, int32_t steps) {
                        if (direction == 0) {
                            carCtrl.setCtrlSteps(0, steps);
                        } else if (direction == 1) { //left/right
                            if (carCtrl.getMotorNum() < 4) { // 2 motor not support
                                out << "not support left/right moving with 2 motor" << "\n";
                            } else {
                                carCtrl.setCtrlSteps(1, -steps);
                                carCtrl.setCtrlSteps(2, steps);
                                carCtrl.setCtrlSteps(3, steps);
                                carCtrl.setCtrlSteps(4, -steps);
                            }
                        } else { //rotation
                            if (carCtrl.getMotorNum() < 4) { // 2 motor not support
                                out << "not support rotation moving with 2 motor" << "\n";
                            } else {
                                carCtrl.setCtrlSteps(1, steps);
                                carCtrl.setCtrlSteps(2, -steps);
                                carCtrl.setCtrlSteps(3, steps);
                                carCtrl.setCtrlSteps(4, -steps);
                            }
                        }
                    },
                    "set car steps");
    cliMenu->insert("set-runtime", {"time: seconds"},
                    [&](std::ostream& out, int32_t runtime) {
                        carCtrl.setRunTime(runtime);
                    },
                    "set car run time for speed regulation");
    cliMenu->insert("stop", [&](std::ostream& out) {
                        carCtrl.setAllMotorState(MOTOR_STATE_STOP);
                    },
                    "stop all motors");

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}

void CliCar::showParam(std::ostream& out)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

    out << "speed level: " << carCtrl.getMotorSpeedLevel() << "\n";
    for (int32_t i = 0; i < carCtrl.getMotorNum(); i++) {
        int32_t actualSpeed = carCtrl.getActualSpeed(i);
        out << "motor " << i+1 << " actual speed=" << actualSpeed << " pwm=" << carCtrl.getMotorPwm(i)
            << " step=" << carCtrl.getActualSteps(i) << "\n";
    }
}

void CliCar::showSteps(std::ostream& out)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

    for (int32_t i = 0; i < carCtrl.getMotorNum(); i++) {
        out << "motor " << i+1 << " steps: control=" << carCtrl.getCtrlSteps(i)
            << " actual=" << carCtrl.getActualSteps(i) << "\n";
    }
}

}  // namespace cli
