// SPDX-License-Identifier: GPL-2.0

#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <cli/cli_impl.h>
#include <cli/cli_car.hpp>
#include <video/sound_intf.hpp>

namespace cli
{
CliCar::CliCar(): CliCommandGroup("car")
{
}

CliCar::~CliCar()
{
    m_client.stop();
}

void CliCar::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto cliMenu = std::make_unique<Menu>(name);
    auto& soundIntf = cmn::getSingletonInstance<SoundIntf>();

    auto setMotorSteps = [&](std::ostream& out, int32_t id, int32_t step) {
        m_client.async_call([&](int32_t res) {
            if (asio2::get_last_error()) {
                out << "rpc setCtrlSteps failed: " << asio2::last_error_val() << ","
                    << asio2::last_error_msg().c_str() << "\n";
            }
        }, "setCtrlSteps", id, step);
    };

    initRpcClient();

    cliMenu->insert("show-param", 
                    [&](std::ostream& out) {
                        int32_t motorNum = m_client.call<int32_t>(std::chrono::seconds(3), "getMotorNum");
                        int32_t speedLevel = m_client.call<int32_t>(std::chrono::seconds(3), "getMotorSpeedLevel");
                        out << "speed level: " << speedLevel << "\n";
                        out << "motor number: " << motorNum << "\n";

                        for (int32_t i = 0; i < motorNum; i++) {
                            int32_t actualSpeed = m_client.call<int32_t>("getActualSpeed", i);
                            int32_t actualStep = m_client.call<int32_t>("getActualSteps", i);
                            int32_t motorPwm = m_client.call<int32_t>("getMotorPwm", i);
                            out << "motor " << i+1 << " speed=" << actualSpeed << " pwm=" << motorPwm
                                << " step=" << actualStep << "\n";
                        }
                    },
                    "show car speed/pwm/step");

    cliMenu->insert("set-speedlevel", {"speed: 1-9 for speed level"},
                    [&](std::ostream& out, int32_t speed) {
                        m_client.async_call([&](int res) {
                            if (asio2::get_last_error()) {
                                out << "rpc setMotorSpeedLevel failed: " << asio2::last_error_val() << ","
                                    << asio2::last_error_msg().c_str() << "\n";
                            }
                        }, "setMotorSpeedLevel", speed);
                    },
                    "set motor speed level");

    cliMenu->insert("set-motorpwm", {"motor:0-all,1~4 motor id", "pwm:0~100"},
                    [&](std::ostream& out, int32_t motor, int32_t pwm) {
                        int32_t motorNum = m_client.call<int32_t>("getMotorNum");
                        if ((motor < 0) || (motor > motorNum) || (pwm > 100)) {
                            out << "paramet error: motor<0-4>-" << motor << " pwm<0-100>-" << pwm << "\n";
                            return;
                        }

                        if (motor == 0) {
                            for (int32_t i = 0; i < motorNum; i++) {
                                m_client.async_call([&]() {
                                    if (asio2::get_last_error()) {
                                        out << "rpc setMotorPwm failed: " << asio2::last_error_val() << ","
                                            << asio2::last_error_msg().c_str() << "\n";
                                    }
							    }, "setMotorPwm", i, pwm);
                            }
                        } else {
                            m_client.async_call([&]() {
                                if (asio2::get_last_error()) {
                                    out << "rpc setMotorPwm failed: " << asio2::last_error_val() << ","
                                        << asio2::last_error_msg().c_str() << "\n";
                                }
							}, "setMotorPwm", motor - 1, pwm);
                        }
                    },
                    "set motor pwm");

    cliMenu->insert("show-motorstep",
                    [&](std::ostream& out) {
                        int32_t motorNum = m_client.call<int32_t>("getMotorNum");
                        for (int32_t i = 0; i < motorNum; i++) {
                            int32_t actualStep = m_client.call<int32_t>("getActualSteps", i);
                            int32_t ctrlStep = m_client.call<int32_t>("getCtrlSteps", i);
                            out << "motor " << i+1 << " steps: control=" << ctrlStep
                                << " actual=" << actualStep << "\n";
                        }
                    },
                    "show car steps");

    cliMenu->insert("set-motorstep", {"wheel:0-all,1~4 motor id", "steps"},
                    [&](std::ostream& out, int32_t wheel, int32_t steps) {
                        int32_t motorNum = m_client.call<int32_t>("getMotorNum");
                        if ((wheel < 0) || (wheel > motorNum) || (steps == 0)) {
                            out << "paramet error: wheel<0-4>-" << wheel << "\n";
                            return;
                        }

                        setMotorSteps(out, wheel, steps);

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
                    {"direction:0-forward/backward, 1-left/right, 2-rotation", "steps: >0-forward or left, <0-backward or right"},
                    [&](std::ostream& out, int32_t direction, int32_t steps) {
                        int32_t motorNum = m_client.call<int32_t>("getMotorNum");
                        if (direction == 0) {
                            setMotorSteps(out, 0, steps);
                        } else if (direction == 1) { //left/right
                            if (motorNum < 4) { // 2 motor not support
                                out << "not support left/right moving with 2 motor" << "\n";
                            } else {
                                setMotorSteps(out, 1, -steps);
                                setMotorSteps(out, 2, steps);
                                setMotorSteps(out, 3, steps);
                                setMotorSteps(out, 4, -steps);
                            }
                        } else { //rotation
                            if (motorNum < 4) { // 2 motor not support
                                out << "not support rotation moving with 2 motor" << "\n";
                            } else {
                                setMotorSteps(out, 1, steps);
                                setMotorSteps(out, 2, -steps);
                                setMotorSteps(out, 3, steps);
                                setMotorSteps(out, 4, -steps);
                            }
                        }
                    },
                    "set car steps");
    cliMenu->insert("set-runtime", {"time: seconds"},
                    [&](std::ostream& out, int32_t runtime) {
                        m_client.async_call([&](int res) {
                            if (asio2::get_last_error()) {
                                out << "rpc setRunTime failed: " << asio2::last_error_val() << ","
                                    << asio2::last_error_msg().c_str() << "\n";
                            }
                        }, "setRunTime", runtime);
                    },
                    "set car run time for speed regulation");
    cliMenu->insert("stop",
                    [&](std::ostream& out) {
                        m_client.async_call([&]() {
                            if (asio2::get_last_error()) {
                                out << "rpc setRunTime failed: " << asio2::last_error_val() << ","
                                    << asio2::last_error_msg().c_str() << "\n";
                            }
                        }, "setAllMotorState", 0);
                    },
                    "stop all motors");

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}


void CliCar::initRpcClient()
{
    m_client.set_default_timeout(std::chrono::seconds(10));

    m_client.bind_connect([&]() {
        if (asio2::get_last_error())
            ctrllog::warn("initRpcClient connect error: {}, {}", asio2::last_error_val(), asio2::last_error_msg().c_str());
        else
            ctrllog::warn("initRpcClient connected");
    });

    m_client.start("127.0.0.0", "8801", asio2::use_dgram);
}

asio2::rpc_client& CliCar::getClient()
{
    return m_client;
}

}  // namespace cli
