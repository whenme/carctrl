// SPDX-License-Identifier: GPL-2.0
#include <sys/stat.h>
#include <xapi/cmn_singleton.hpp>
#include <xapi/easylog.hpp>
#include <xapi/iotimer.hpp>
#include <xapi/pty_shell.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include <rpc_service/rpc_service.hpp>
#include "car_ctrl.hpp"

int32_t main(int argc, char **argv)
{
    pty_shell_init(2);
    init_log();

    coro_rpc::coro_rpc_server coro_server(std::thread::hardware_concurrency(), rpc_port);
    auto& asioContext = coro_server.get_io_context_pool().get_executor()->context();

    CarCtrl carCtrl{asioContext};
    cmn::setSingletonInstance(&carCtrl);

    coro_server.register_handler<getActualSpeed, setCtrlSteps, getCtrlSteps,
                                 getActualSteps, setRunTime, setMotorSpeedLevel,
                                 getMotorSpeedLevel, setAllMotorState, getMotorNum,
                                 getMotorPwm, setCarSteps, setCarMoving, setSteerTurn>();

    auto timerCallback = [](const asio::error_code &e, void *ctxt)
    {
        static int state = 0;
        struct stat buffer;
        if (stat("/sys/class/leds/orangepi:green:status/brightness", &buffer) == 0) {
            if (state == 0) {
                system("echo 1 > /sys/class/leds/orangepi:green:status/brightness");
                state = 1;
            } else {
                system("echo 0 > /sys/class/leds/orangepi:green:status/brightness");
                state = 0;
            }
        }
    };
    IoTimer timer(asioContext, timerCallback, nullptr, true);
    timer.start(1000);

    auto res = coro_server.start();
    if (res != coro_rpc::errc::ok) {
        ctrllog::error("failed to start coro_rpc server...");
    }
    return 0;
}
