// SPDX-License-Identifier: GPL-2.0
#include <exception>
#include <functional>

#include <xapi/cmn_singleton.hpp>
#include <xapi/easylog.hpp>
#include <xapi/pty_shell.hpp>
#include <cli_impl.h>
#include <remote_key.hpp>
#include <video/sound_intf.hpp>
#include <video/video_ctrl.hpp>

int32_t main(int argc, char **argv)
{
    auto cleanupExit = []() {
        pty_shell_exit();
    };

    pty_shell_init(2);
    init_log();

    std::set_terminate(cleanupExit);
    std::atexit(cleanupExit);
    std::at_quick_exit(cleanupExit);

    SoundIntf soundIntf;
    cmn::setSingletonInstance(&soundIntf);

    cli::CliImpl cliImpl;
    cmn::setSingletonInstance(&cliImpl);

    RemoteKey remoteKey{cliImpl.getIoContext()};
    cmn::setSingletonInstance(&remoteKey);

    VideoCtrl videoCtrl{cliImpl.getIoContext()};
    cmn::setSingletonInstance(&videoCtrl);

    cliImpl.initCliCommand();
    cliImpl.runCliImpl();

    return 0;
}
