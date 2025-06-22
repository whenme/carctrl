// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <string>

namespace cmn
{
    void printStackTrace();
    void printStackTraceExit();

    void notSupported(std::string_view userMsg = "");
    [[noreturn]] void notSupportedExit(std::string_view userMsg = "");

    void notImplemented();
    [[noreturn]] void notImplementedExit();
}  // namespace cmn

#ifndef NDEBUG
#define CMN_ASSERT(...)             \
    if (!(__VA_ARGS__))             \
    {                               \
        cmn::printStackTraceExit(); \
    }                               \
    assert(__VA_ARGS__)
#else
#define CMN_ASSERT(...) assert(__VA_ARGS__)
#endif
