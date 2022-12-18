// SPDX-License-Identifier: GPL-2.0
#include <sys/stat.h>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/easylog.hpp>
#include <ioapi/iotimer.hpp>
#include <cli/cli_impl.h>
#include <video/sound_intf.hpp>
#include <video/video_ctrl.hpp>

#include "car_ctrl.hpp"
#include "remote_key.hpp"

int32_t main(int argc, char **argv)
{
    easylog::init_log();

    SoundIntf soundIntf;
    cmn::setSingletonInstance(&soundIntf);

    cli::CliImpl cliImpl;
    cmn::setSingletonInstance(&cliImpl);

    CarCtrl carCtrl{cliImpl.getIoContext()};
    cmn::setSingletonInstance(&carCtrl);

    RemoteKey remoteKey{cliImpl.getIoContext()};
    cmn::setSingletonInstance(&remoteKey);

    VideoCtrl videoCtrl{cliImpl.getIoContext()};
    cmn::setSingletonInstance(&videoCtrl);

    auto timerCallback = [](const asio::error_code &e, void *ctxt)
    {
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
    };
    IoTimer timer(cliImpl.getIoContext(), timerCallback, nullptr, true);
    timer.start(1000);

    cliImpl.initCliCommand();
    cliImpl.runCliImpl();
    return 0;
}
