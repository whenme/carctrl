// SPDX-License-Identifier: GPL-2.0

#include <xapi/cmn_singleton.hpp>
#include <cli_impl.h>
#include <video/cli_video.hpp>
#include <video/sound_intf.hpp>

namespace cli
{
void CliVideo::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name = getGroupName();
    auto  cliMenu = std::make_unique<Menu>(name);
    auto&     obj = cmn::getSingletonInstance<SoundIntf>();

    cliMenu->Insert("set-sound",
                    [&](std::ostream& out, int state) {
                        obj.setSoundState(state);
                    },
                    "set sound state on/off",
                    {"state:0-off, 1-on"});
    cliMenu->Insert("sing", [&](std::ostream&) {
                        obj.sing();
                    },
                    "sing");

    if (rootMenu != nullptr) {
        rootMenu->Insert(std::move(cliMenu));
    }
}

}  // namespace cli
