// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <xapi/libfork.hpp>

namespace cmn {

template<typename Func, typename... Args>
auto wrapperFunc(Func&& func, Args&&... args)
    requires std::invocable<Func, Args...>
{
    return [...args = std::forward<Args>(args), &func]() mutable -> decltype(auto) {
        if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
            std::forward<Func>(func)(std::forward<Args>(args)...);
        } else {
            auto result = std::forward<Func>(func)(std::forward<Args>(args)...);
            return result;
        }
    };
}

template<typename Func, typename... Args>
auto wrapperTaskFun(Func&& func, Args&& ...args) -> lf::task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        co_await lf::fork(lf::lift)(std::forward<Func>(func), std::forward<Args>(args)...);
    } else {
        lf::eventually<std::invoke_result_t<Func, Args...>> ret;
        co_await lf::fork(&ret, lf::lift)(std::forward<Func>(func), std::forward<Args>(args)...);
        co_return ret;
    }
};

template<typename Func, typename... Args>
auto wrapperTaskFork(Func&& func, Args&& ...args) -> lf::task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        //co_await lf::fork(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::just(std::forward<Func>(func), std::forward<Args>(args)...);
        co_return;
    } else {
        lf::eventually<std::invoke_result_t<Func, Args...>> ret;

        //co_return co_await lf::just(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::fork(&ret, std::forward<Func>(func))(std::forward<Args>(args)...);
        co_return ret;
    }
};

template<typename Func, typename... Args>
auto wrapperTaskForkJoin(Func&& func, Args&& ...args) -> lf::task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        co_await lf::fork(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::join;
        co_return;
    } else {
        lf::eventually<std::invoke_result_t<Func, Args...>> ret;

        co_await lf::fork(&ret, std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::join;
        co_return ret;
    }
};

} //namespace cmn
