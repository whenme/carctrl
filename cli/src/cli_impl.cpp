// SPDX-License-Identifier: GPL-2.0

#include <cli/cli_impl.h>
#include <cli/cli_example.h>
#include <cli/cli_car.hpp>
#include <cli/cli_video.hpp>
#include <ioapi/cmn_singleton.hpp>

#include <algorithm>
#include <vector>

using namespace cli;

CliImpl::~CliImpl()
{
    m_cliGroup.clear();
}

void CliImpl::initCliCommand()
{
    auto addCommandGroup = [&](CliCommandGroup& cmd) {
        for (auto& it : m_cliGroup) {
             // already added
            if (it->getGroupName() == cmd.getGroupName())
                return;
        }
        m_cliGroup.push_back(&cmd);
    };

    static CliExample cliExample;
    static CliCar     cliCar;
    static CliVideo   cliVideo;

    cmn::setSingletonInstance<CliCar>(&cliCar);

    addCommandGroup(cliExample);
    addCommandGroup(cliCar);
    addCommandGroup(cliVideo);

    // setup cli
    for (auto& it : m_cliGroup) {
        it->initCliCommand(m_rootMenu);
    }
}

void CliImpl::runCliImpl()
{
    m_cli = std::make_unique<Cli>(std::move(m_rootMenu), std::make_unique<FileHistoryStorage>("cli.log"));
    // global exit action
    m_cli->exitAction([](auto& out) {
        out << "Goodbye and thanks for all the fish.\n";
    });
    // std exception custom handler
    m_cli->stdExceptionHandler([](std::ostream& out, const std::string& cmd, const std::exception& err) {
        out << "Exception caught in cli handler: " << err.what() << " handling command: " << cmd << ".\n";
    });

    detail::CliLocalTerminalSession localSession(*m_cli, m_scheduler, std::cout, k_MLocalHistorySize);
    localSession.exitAction([&](auto& out) { // session exit action
                                out << "Closing App...\n";
                                m_scheduler.stop();
                            });

    // setup server
    CliTelnetServer server(*m_cli, m_scheduler, k_MTelnetServerPort);
    // exit action for all the connections
    server.exitAction([](auto& out) {
        out << "Terminating this session...\n";
    });

    m_scheduler.run();
}

asio::io_context& CliImpl::getIoContext()
{
    return m_scheduler.getAsioContext();
}
