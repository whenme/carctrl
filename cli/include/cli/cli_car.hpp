// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cli/cli.h>
#include <cli/cli_impl.h>
#include <memory>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

namespace cli
{
class CliCommandGroup;

class CliCar : public CliCommandGroup
{
public:
    CliCar();
    virtual ~CliCar();

    void initCliCommand(std::unique_ptr<Menu>& rootMenu) override;
    coro_rpc::coro_rpc_client& getClient();

private:
    coro_rpc::coro_rpc_client m_client;
};

}  // namespace cli
