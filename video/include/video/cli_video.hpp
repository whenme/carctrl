// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cli/cli.h>
#include <cli/cli_impl.h>
#include <memory>

namespace cli
{
class CliCommandGroup;

class CliVideo : public CliCommandGroup
{
public:
    CliVideo() : CliCommandGroup("video")
    {
    }
    virtual ~CliVideo() = default;

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;

private:
    //static void setSoundState(std::ostream& out, int32_t state);
};

}  // namespace cli
