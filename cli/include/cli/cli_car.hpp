// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cli/cli.h>
#include <cli/cli_impl.h>
#include <memory>
#include <asio2/asio2.hpp>

namespace cli
{
class CliCommandGroup;

class CliCar : public CliCommandGroup
{
public:
    CliCar();
    virtual ~CliCar();

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;
    asio2::rpc_client& getClient();

private:
    asio2::rpc_client m_client;

    void initRpcClient();
};

}  // namespace cli
