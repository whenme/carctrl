// SPDX-License-Identifier: GPL-2.0

#include <cli_impl.h>
#include <cli_example.h>
#include <cli_car.hpp>
#include <cli_video.hpp>
#include <cli/filehistorystorage.h>
#include <cli/clilocalsession.h>
#include <xapi/cmn_singleton.hpp>

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

    //static CliExample cliExample;
    static CliCar     cliCar;
    static CliVideo   cliVideo;

    cmn::setSingletonInstance<CliCar>(&cliCar);

    //addCommandGroup(cliExample);
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
    m_cli->ExitAction([](auto& out) {
        out << "Goodbye and thanks for all the fish.\n";
    });
    // std exception custom handler
    m_cli->StdExceptionHandler([](std::ostream& out, const std::string& cmd, const std::exception& err) {
        out << "Exception caught in cli handler: " << err.what() << " handling command: " << cmd << ".\n";
    });

    CliLocalTerminalSession localSession(*m_cli, m_scheduler, std::cout, k_MLocalHistorySize);
    localSession.ExitAction([&](auto& out) { // session exit action
                                out << "Closing App...\n";
                                m_scheduler.Stop();
                            });

    // setup server
    CliTelnetServer server(*m_cli, m_scheduler, k_MTelnetServerPort);
    // exit action for all the connections
    server.ExitAction([](auto& out) {
        out << "Terminating this session...\n";
    });

    m_scheduler.Run();
}

asio::io_context& CliImpl::getIoContext()
{
    return m_scheduler.AsioContext();
}
