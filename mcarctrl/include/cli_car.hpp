// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cli/cli.h>
#include <cli/cli_impl.h>
#include <memory>

namespace cli
{
class CliCommandGroup;

class CliCar : public CliCommandGroup
{
public:
    CliCar() : CliCommandGroup("car")
    {
    }
    virtual ~CliCar() = default;

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;

private:
    static void showParam(std::ostream& out);
    static void showSteps(std::ostream& out);

    static constexpr int32_t m_maxSpeed = 60;
};

}  // namespace cli
