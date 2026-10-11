// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <memory>
#include <string>
#include <cli/cli.h>

namespace cli
{

class CliCommandGroup
{
public:
    explicit CliCommandGroup(std::string name) : m_name(std::move(name))
    {
    }
    virtual ~CliCommandGroup() = default;

    virtual void initCliCommand(std::unique_ptr<Menu>& rootMenu) = 0;

    std::string& getGroupName()
    {
        return m_name;
    }

private:
    std::string m_name;
};

}  // namespace cli
