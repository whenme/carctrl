// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLI_IMPL_H_INCLUDE__
#define __CLI_CLI_IMPL_H_INCLUDE__

#include "cli.h"

namespace cli
{
using MainScheduler   = detail::GenericAsioScheduler<detail::StandaloneAsioLib>;
using CliTelnetServer = detail::CliGenericTelnetServer<detail::StandaloneAsioLib>;
using CliAsyncSession = detail::GenericCliAsyncSession<detail::StandaloneAsioLib>;

class CliCommandGroup
{
public:
    explicit CliCommandGroup(std::string name) : m_name(std::move(name))
    {
    }
    virtual ~CliCommandGroup() = default;

    CliCommandGroup(const CliCommandGroup&)            = delete;
    CliCommandGroup& operator=(const CliCommandGroup&) = delete;
    CliCommandGroup(CliCommandGroup&&)                 = delete;
    CliCommandGroup& operator=(CliCommandGroup&&)      = delete;

    virtual void initCliCommand(std::unique_ptr<Menu>& rootMenu) = 0;

    std::string& getGroupName()
    {
        return m_name;
    }

private:
    std::string m_name;
};

class CliImpl
{
public:
    CliImpl() = default;
    virtual ~CliImpl();

    CliImpl(const CliImpl&)            = delete;
    CliImpl& operator=(const CliImpl&) = delete;
    CliImpl(CliImpl&&)                 = delete;
    CliImpl& operator=(CliImpl&&)      = delete;

    bool addCommandGroup(CliCommandGroup* cmd);
    void initCliCommand();
    void runCliImpl();

private:
    std::unique_ptr<Menu>         m_rootMenu = std::make_unique<Menu>("cli");
    std::unique_ptr<Cli>          m_cli;
    MainScheduler                 m_scheduler;
    std::vector<CliCommandGroup*> m_cliGroup;

    static constexpr uint16_t    k_MTelnetServerPort = 5000;
    static constexpr std::size_t k_MLocalHistorySize = 200;
};

}  // namespace cli

#endif  // __CLI_CLI_IMPL_H_INCLUDE__
