// SPDX-License-Identifier: GPL-2.0
#include <sys/stat.h>
#include <asio2/asio2.hpp>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <ioapi/iotimer.hpp>
#include <ioapi/pty_shell.hpp>

#include "car_ctrl.hpp"

void initRpcServer(asio2::rpc_server& server, CarCtrl& ctrl)
{
    server.bind_connect([&](auto &session_ptr) {
        ctrllog::warn("client enter: remote {}:{}, local {}:{}",
            session_ptr->remote_address().c_str(), session_ptr->remote_port(),
            session_ptr->local_address().c_str(), session_ptr->local_port());
    }).bind_start([&]() {
        ctrllog::warn("start rpc server: {}:{}",
            server.listen_address().c_str(), server.listen_port());
    });

    server.bind("getActualSpeed", &CarCtrl::getActualSpeed, ctrl)
          .bind("setCtrlSteps", &CarCtrl::setCtrlSteps, ctrl)
          .bind("getCtrlSteps", &CarCtrl::getCtrlSteps, ctrl)
          .bind("getActualSteps", &CarCtrl::getActualSteps, ctrl)
          .bind("setRunTime", &CarCtrl::setRunTime, ctrl)
          .bind("setMotorSpeedLevel", &CarCtrl::setMotorSpeedLevel, ctrl)
          .bind("getMotorSpeedLevel", &CarCtrl::getMotorSpeedLevel, ctrl)
          .bind("setAllMotorState", &CarCtrl::setAllMotorState, ctrl)
          .bind("getMotorNum", &CarCtrl::getMotorNum, ctrl);
}

int32_t main(int argc, char **argv)
{
    pty_shell_init(2);

    asio2::rpc_server server(512,  // the initialize recv buffer size
		                     1024, // the max recv buffer size
		                     4);   // the thread count

    init_log();

    CarCtrl carCtrl{server};
    cmn::setSingletonInstance(&carCtrl);

    initRpcServer(server, carCtrl);
    //auto& context=server.get_io_context();
    server.start_timer("led", 1, [&]() {
        static int state = 0;
        struct stat buffer;
        if (stat("/sys/class/leds/orangepi:red:status/brightness", &buffer) == 0) {
            if (state == 0) {
                system("echo 1 > /sys/class/leds/orangepi:red:status/brightness");
                state = 1;
            } else {
                system("echo 0 > /sys/class/leds/orangepi:red:status/brightness");
                state = 0;
            }
        }
    });

    server.start("0.0.0.0", "8801", asio2::use_dgram);

    while(1)
        sleep(10);

    return 0;
}
