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

    cliMenu->insert("show-speed", showSpeed, "show car speed");
    cliMenu->insert("set-speed", {"speed: 1-9 for speed level"},
                    [&](std::ostream& out, int32_t speed) {
                        carCtrl.setMotorSpeedLevel(speed);
                    },
                    "set motor speed level");

    cliMenu->insert("set-pwm", {"motor:0-all,1~4 motor id", "pwm:0~100"},
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

    cliMenu->insert("show-step", showSteps, "show car steps");
    cliMenu->insert("set-step", {"wheel:0-all,1~4 motor id", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        if ((wheel < 0) || (wheel > carCtrl.getMotorNum()) || (steps == 0)) {
                            out << "paramet error: wheel<0-2>-" << wheel << "\n";
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
                                sprintf(sound, "左轮前进%d步", steps);
                            else
                                sprintf(sound, "左轮后退%d步", abs(steps));
                        } else if (wheel == 2) {
                            carCtrl.setCtrlSteps(wheel, steps);
                            if (steps > 0)
                                sprintf(sound, "右轮前进%d步", steps);
                            else
                                sprintf(sound, "右轮后退%d步", abs(steps));
                        }
                        soundIntf.speak(sound);
                    },
                    "set car steps");

    cliMenu->insert("set-runtime", {"time: seconds"},
                    [&](std::ostream& out, int32_t runtime) {
                        carCtrl.setRunTime(runtime);
                    },
                    "set car run time");

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}

void CliCar::showSpeed(std::ostream& out)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

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
