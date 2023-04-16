// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLI_EXAMPLE_H_INCLUDE__
#define __CLI_CLI_EXAMPLE_H_INCLUDE__

#pragma once
#include <cli/cli.h>
#include <cli/cli_impl.h>
#include <memory>

namespace cli
{
class CliCommandGroup;

class CliExample : public CliCommandGroup
{
public:
    CliExample() : CliCommandGroup("example")
    {
    }
    virtual ~CliExample() = default;

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;
};

}  // namespace cli

#endif  // __CLI_CLI_EXAMPLE_H_INCLUDE__
