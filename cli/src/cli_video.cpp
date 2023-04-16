// SPDX-License-Identifier: GPL-2.0

#include <ioapi/cmn_singleton.hpp>
#include <cli/cli_impl.h>
#include <cli/cli_video.hpp>
#include <video/sound_intf.hpp>

namespace cli
{
void CliVideo::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto  cliMenu = std::make_unique<Menu>(name);
    auto&     obj = cmn::getSingletonInstance<SoundIntf>();

    cliMenu->insert("set-sound",
                    [&](std::ostream& out, int state) {
                        obj.setSoundState(state);
                    },
                    "set sound state on/off",
                    {"state:0-off, 1-on"});
    cliMenu->insert("sing", [&](std::ostream&) {
                        obj.sing();
                    },
                    "sing");

    if (rootMenu != nullptr) {
        rootMenu->insert(std::move(cliMenu));
    }
}

}  // namespace cli
