// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <xapi/libfork.hpp>

namespace lf {

template<typename Func, typename... Args>
auto wrapper(auto, Func&& func, Args&& ...args)
        -> task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        co_await lf::just(std::forward<Func>(func))(std::forward<Args>(args)...);
    } else {
        co_return co_await lf::just(std::forward<Func>(func))(std::forward<Args>(args)...);
    }
};

template<typename Func, typename... Args>
auto wrapperFork(Func&& func, Args&& ...args) -> task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    lf::eventually<std::invoke_result_t<Func, Args...>> ret;

    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        //co_await lf::fork(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::fork(lf::lift)(std::forward<Func>(func), std::forward<Args>(args)...);
        co_return;
    } else {
        //co_return co_await lf::just(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::fork(&ret, lf::lift)(std::forward<Func>(func), std::forward<Args>(args)...);
        co_return ret;
    }
};

template<typename Func, typename... Args>
auto wrapperForkJoin(Func&& func, Args&& ...args) -> task<std::invoke_result_t<Func, Args...>>
    requires std::invocable<Func, Args...>
{
    lf::eventually<std::invoke_result_t<Func, Args...>> ret;

    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        co_await lf::fork(std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::join;
        co_return;
    } else {
        co_await lf::fork(&ret, std::forward<Func>(func))(std::forward<Args>(args)...);
        co_await lf::join;
        co_return ret;
    }
};

auto wrapper0 = []<typename Func>(auto, Func&& func)
        -> task<std::invoke_result_t<Func>>
    requires std::invocable<Func>
{
    if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
        co_await lf::just(std::forward<Func>(func))();
    } else {
        co_return co_await lf::just(std::forward<Func>(func))();
    }
};


} //namespace lf
