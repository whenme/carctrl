// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLI_IMPL_H_INCLUDE__
#define __CLI_CLI_IMPL_H_INCLUDE__

#pragma once
#include "asio.hpp"
#include "cli/cli.h"
#include "cli/detail/genericasioscheduler.h"
#include "cli/detail/genericasioremotecli.h"
#include "cli/detail/genericcliasyncsession.h"

namespace cli
{
using MainScheduler   = detail::GenericAsioScheduler<detail::NewStandaloneAsioLib>;
using CliTelnetServer = detail::CliGenericTelnetServer<detail::NewStandaloneAsioLib>;
using CliAsyncSession = detail::GenericCliAsyncSession<detail::NewStandaloneAsioLib>;

class CliCommandGroup
{
public:
    explicit CliCommandGroup(std::string name) : m_name(std::move(name))
    {
    }
    virtual ~CliCommandGroup() = default;

    //class are neither copyable nor movable
    //CMN_UNCOPYABLE_IMMOVABLE(CliCommandGroup)

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

    //class are neither copyable nor movable
    //CMN_UNCOPYABLE_IMMOVABLE(CliImpl)

    void initCliCommand();
    void runCliImpl();
    asio::io_context& getIoContext();

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
