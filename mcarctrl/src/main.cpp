// SPDX-License-Identifier: GPL-2.0
#include <sys/stat.h>
#include <ioapi/cmn_singleton.hpp>
#include <ioapi/iotimer.hpp>
#include <ioapi/iothread_pool.hpp>
#include <cli/cli_impl.h>
#include <video/sound_intf.hpp>

#include "car_ctrl.hpp"
#include "remote_key.hpp"
#include "car_speed.hpp"


int32_t main(int argc, char **argv)
{
    SoundIntf soundIntf;
    cmn::setSingletonInstance(&soundIntf);
    soundIntf.speak("你好, 上海");
    soundIntf.speak("南汇第二中学, 人工智能车");

    cli::CliImpl cliImpl;
    cmn::setSingletonInstance(&cliImpl);
    auto& cliObj = cmn::getSingletonInstance<cli::CliImpl>();

    CarCtrl carCtrl{cliObj.getIoContext()};
    cmn::setSingletonInstance(&carCtrl);

    RemoteKey remoteKey{cliObj.getIoContext()};
    cmn::setSingletonInstance(&remoteKey);

    CarSpeed carSpeed{cliObj.getIoContext()};
    cmn::setSingletonInstance(&carSpeed);

    auto timerCallback = [&](const asio::error_code &e, void *ctxt)
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
    IoTimer timer(cliObj.getIoContext(), timerCallback, nullptr, true);
    timer.start(1000);

    cliObj.initCliCommand();
    cliObj.runCliImpl();
    return 0;
}
