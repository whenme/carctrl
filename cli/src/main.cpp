// SPDX-License-Identifier: GPL-2.0
#include <xapi/cmn_singleton.hpp>
#include <xapi/easylog.hpp>
#include <xapi/pty_shell.hpp>
#include <cli/cli_impl.h>
#include <cli/remote_key.hpp>
#include <video/sound_intf.hpp>
#include <video/video_ctrl.hpp>

int32_t main(int argc, char **argv)
{
    pty_shell_init(2);
    init_log();

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
