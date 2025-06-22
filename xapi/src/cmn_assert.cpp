// SPDX-License-Identifier: GPL-2.0
#include <xapi/cmn_assert.hpp>

#include <fmt/format.h>
#include <exception>
#include <stacktrace>

[[noreturn]] static inline void terminate_app()
{
    fmt::print(stderr, "Terminating the program.\n");
    std::terminate();
}

void cmn::printStackTrace()
{
    fmt::print(stderr, "   {}\n", std::stacktrace::current());
}

void cmn::printStackTraceExit()
{
    cmn::printStackTrace();
    terminate_app();
}

void cmn::notSupported(std::string_view userMsg)
{
    if (userMsg.empty())
        fmt::print(stderr, "Not supported; stack trace:\n");
    else
        fmt::print(stderr, "Not supported: {}; stack trace:\n", userMsg);

    cmn::printStackTrace();
}

[[noreturn]] void cmn::notSupportedExit(std::string_view userMsg)
{
    cmn::notSupported(userMsg);
    terminate_app();
}

void cmn::notImplemented()
{
    fmt::print(stderr, "Not implemented; stack trace:\n");

    cmn::printStackTrace();
}

[[noreturn]] void cmn::notImplementedExit()
{
    cmn::notImplemented();
    terminate_app();
}
