// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cli_command_group.hpp>
#include <memory>

namespace cli
{

class CliCar : public CliCommandGroup
{
public:
    CliCar();
    virtual ~CliCar() = default;

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;
    void quitApplications();
};

}  // namespace cli
