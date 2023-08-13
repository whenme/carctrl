// SPDX-License-Identifier: GPL-2.0

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <cli/cli_impl.h>
#include <cli/cli_car.hpp>
#include <video/sound_intf.hpp>
#include <rpc_service/rpc_service.hpp>

using namespace async_simple::coro;

namespace cli
{
CliCar::CliCar(): CliCommandGroup("car")
{
}

CliCar::~CliCar()
{
}

void CliCar::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto cliMenu = std::make_unique<Menu>(name);
    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();

    syncAwait(m_client.connect("localhost", "8802"));

    cliMenu->insert("show-param",
                    [&](std::ostream& out) {
                        int32_t motorNum = rpc_call_int_param<getMotorNum>(m_client);
                        int32_t speedLevel = rpc_call_int_param<getMotorSpeedLevel>(m_client);
                        out << "speed level: " << speedLevel << "\n";
                        out << "motor number: " << motorNum << "\n";

                        for (int32_t i = 0; i < motorNum; i++) {
                            int32_t actualSpeed = rpc_call_int_param<getActualSpeed>(m_client, i);
                            int32_t actualStep = rpc_call_int_param<getActualSteps>(m_client, i);
                            int32_t motorPwm = rpc_call_int_param<getMotorPwm>(m_client, i);
                            out << "motor " << i+1 << " speed=" << actualSpeed << " pwm=" << motorPwm
                                << " step=" << actualStep << "\n";
                        }
                    },
                    "show car speed/pwm/step");

    cliMenu->insert("set-speedlevel", {"speed: 1-9 for speed level"},
                    [&](std::ostream& out, int32_t speed) {
                        rpc_call_int_param<setMotorSpeedLevel>(m_client, speed);
                    },
                    "set motor speed level");

    cliMenu->insert("set-motorpwm", {"motor:0-all,1~4 motor id", "pwm:0~100"},
                    [&](std::ostream& out, int32_t motor, int32_t pwm) {
                        int32_t motorNum = rpc_call_int_param<getMotorNum>(m_client);
                        if ((motor < 0) || (motor > motorNum) || (pwm > 100)) {
                            out << "paramet error: motor<0-4>-" << motor << " pwm<0-100>-" << pwm << "\n";
                            return;
                        }

                        if (motor == 0) {
                            for (int32_t i = 0; i < motorNum; i++) {
                                rpc_call_void_param<setMotorPwm>(m_client, i, pwm);
                            }
                        } else {
                            rpc_call_void_param<setMotorPwm>(m_client, motor - 1, pwm);
                        }
                    },
                    "set motor pwm");

    cliMenu->insert("show-motorstep",
                    [&](std::ostream& out) {
                        int32_t motorNum = rpc_call_int_param<getMotorNum>(m_client);
                        for (int32_t i = 0; i < motorNum; i++) {
                            int32_t actualStep = rpc_call_int_param<getActualSteps>(m_client, i);
                            int32_t ctrlStep = rpc_call_int_param<getCtrlSteps>(m_client, i);
                            out << "motor " << i+1 << " steps: control=" << ctrlStep
                                << " actual=" << actualStep << "\n";
                        }
                    },
                    "show car steps");

    cliMenu->insert("set-motorstep", {"wheel:0-all,1~4 motor id", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        int32_t motorNum = rpc_call_int_param<getMotorNum>(m_client);
                        if ((wheel < 0) || (wheel > motorNum) || (steps == 0)) {
                            out << "paramet error: wheel<0-4>-" << wheel << "\n";
                            return;
                        }

                        rpc_call_int_param<setCtrlSteps>(m_client, wheel, steps);

                        char sound[128] = {0};
                        if (wheel == 0) {
                            if (steps > 0)
                                sprintf(sound, "前进%d步", steps);
                            else
                                sprintf(sound, "后退%d步", abs(steps));
                        } else if (wheel == 1) {
                            if (steps > 0)
                                sprintf(sound, "左前轮前进%d步", steps);
                            else
                                sprintf(sound, "左前轮后退%d步", abs(steps));
                        } else if (wheel == 2) {
                            if (steps > 0)
                                sprintf(sound, "右前轮前进%d步", steps);
                            else
                                sprintf(sound, "右前轮后退%d步", abs(steps));
                        } else if (wheel == 3) {
                            if (steps > 0)
                                sprintf(sound, "左后轮前进%d步", steps);
                            else
                                sprintf(sound, "左后轮后退%d步", abs(steps));
                        } else if (wheel == 4) {
                            if (steps > 0)
                                sprintf(sound, "右后轮前进%d步", steps);
                            else
                                sprintf(sound, "右后轮后退%d步", abs(steps));
                        }
                        soundIntf.speak(sound);
                    },
                    "set motor steps");

    cliMenu->insert("set-carstep",
                    {"direction:0-forward/backward, 1-left/right, 2-rotation", "steps: >0-forward/left, <0-backward/right"},
                    [&](std::ostream& out, int32_t direction, int32_t steps) {
                        int32_t motorNum = rpc_call_int_param<getMotorNum>(m_client);
                        if (direction == 0) {
                            rpc_call_int_param<setCtrlSteps>(m_client, 0, steps);
                        } else if (direction == 1) { //left/right
                            if (motorNum < 4) { // 2 motor not support
                                out << "not support left/right moving with 2 motor" << "\n";
                            } else {
                                rpc_call_int_param<setCtrlSteps>(m_client, 1, -steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 2, steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 3, steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 4, -steps);
                            }
                        } else { //rotation
                            if (motorNum < 4) { // 2 motor not support
                                out << "not support rotation moving with 2 motor" << "\n";
                            } else {
                                rpc_call_int_param<setCtrlSteps>(m_client, 1, steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 2, -steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 3, steps);
                                rpc_call_int_param<setCtrlSteps>(m_client, 4, -steps);
                            }
                        }
                    },
                    "set car steps");
    cliMenu->insert("set-runtime", {"time: seconds"},
                    [&](std::ostream& out, int32_t runtime) {
                        rpc_call_int_param<setRunTime>(m_client, runtime);
                    },
                    "set car run time for speed regulation");
    cliMenu->insert("stop",
                    [&](std::ostream& out) {
                        rpc_call_void_param<setAllMotorState>(m_client, 0);
                    },
                    "stop all motors");

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}

coro_rpc::coro_rpc_client& CliCar::getClient()
{
    return m_client;
}

}  // namespace cli
