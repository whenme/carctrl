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

    cliMenu->insert("set-speed",
                    {"wheel:0-both,1-left,2-right", "speed:-60~+60"},
                    [&](std::ostream& out, int32_t wheel, int32_t speed) {
                        if ((wheel < 0) || (wheel > 2) || (speed > 60)) {
                            out << "paramet error: wheel<0-2>-" << wheel << " speed<0-60>-" << speed << "\n";
                            return;
                        }

                        if (wheel == 0) {
                            carCtrl.setCtrlSpeed(0, speed);
                            carCtrl.setCtrlSpeed(1, speed);
                        } else {
                            carCtrl.setCtrlSpeed(wheel - 1, speed);
                        }
                    },
                    "set car speed");

    cliMenu->insert("show-step", showSteps, "show car steps");
    cliMenu->insert("set-step",
                    {"wheel:0-both,1-left,2-right", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        if ((wheel < 0) || (wheel > 2) || (steps == 0)) {
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

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}

void CliCar::showSpeed(std::ostream& out)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

    int32_t ctrlLeft, ctrlRight;
    carCtrl.getCtrlSpeed(0, &ctrlLeft);
    carCtrl.getCtrlSpeed(1, &ctrlRight);
    out << "control speed: left:" << std::dec << ctrlLeft << "   right:" << ctrlRight << "\n";

    int32_t actualLeft = carCtrl.getActualSpeed(0);
    int32_t actualRight = carCtrl.getActualSpeed(1);
    out << "actual speed:  left:" << std::dec << actualLeft << "   right:" << actualRight << "\n";
}

void CliCar::showSteps(std::ostream& out)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();

    int32_t ctrlLeft = carCtrl.getCtrlSteps(0);
    int32_t ctrlRight = carCtrl.getCtrlSteps(1);
    out << "control step: left:" << std::dec << ctrlLeft << "   right:" << ctrlRight << "\n";

    int32_t actualLeft = carCtrl.getActualSteps(0);
    int32_t actualRight = carCtrl.getActualSteps(1);
    out << "actual step:  left:" << std::dec << actualLeft << "   right:" << actualRight << "\n";
}

void CliCar::setCtrlSpeed(std::ostream& out, int32_t wheel, int32_t speed)
{
    auto& carCtrl = cmn::getSingletonInstance<CarCtrl>();
    if ((wheel < 0) || (wheel > 2) || (speed > m_maxSpeed) || (speed < -m_maxSpeed)) {
        out << "paramet error: wheel<0-2>-" << wheel << " speed<0-60>-" << speed << "\n";
        return;
    }

    if (wheel == 0) {
        carCtrl.setCtrlSpeed(0, speed);
        carCtrl.setCtrlSpeed(1, speed);
    } else {
        carCtrl.setCtrlSpeed(wheel - 1, speed);
    }
}

}  // namespace cli
