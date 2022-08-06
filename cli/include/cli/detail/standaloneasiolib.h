// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_STANDALONEASIOLIB_H_INCLUDED__
#define __CLI_DETAIL_STANDALONEASIOLIB_H_INCLUDED__

#include <asio.hpp>
#include <asio/version.hpp>

namespace cli::detail
{
namespace asiolib   = asio;
namespace asiolibec = asio;

class StandaloneAsioLib
{
public:
    using ContextType = asio::io_context;
    using WorkGuard   = asio::executor_work_guard<asio::io_context::executor_type>;

    class Executor
    {
    public:
        explicit Executor(ContextType& ios) : m_executor(ios.get_executor())
        {
        }
        explicit Executor(asio::ip::tcp::socket& socket) : m_executor(socket.get_executor())
        {
        }
        template<typename T>
        void post(T&& temp)
        {
            asio::post(m_executor, std::forward<T>(temp));
        }

    private:
#if ASIO_VERSION >= 101700
        using AsioExecutor = asio::any_io_executor;
#else
        using AsioExecutor = asio::executor;
#endif
        AsioExecutor m_executor;
    };

    static asio::ip::address ipAddressFromString(const std::string& address)
    {
        return asio::ip::make_address(address);
    }

    static auto makeWorkGuard(ContextType& context)
    {
        return asio::make_work_guard(context);
    }
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_STANDALONEASIOLIB_H_INCLUDED__
