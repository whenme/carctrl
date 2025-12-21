// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <future>

namespace cmn
{

/** @return @c true if the specified future's associated shared state is ready, @c false otherwise
 *
 * @param theFuture Future to check for readiness
 *
 * @throw std::future_error if @c theFuture has no associated shared state
 */
template<typename T>
[[nodiscard]] bool isReady(std::future<T>& theFuture)
{
    if (!theFuture.valid())
    {
        throw std::future_error{std::future_errc::no_state};
    }

    const auto status =
        theFuture.wait_for(std::chrono::seconds{0});  // does not actually wait in GCC and MSVC implementations

    return (status == std::future_status::ready);
}

/** @} */
}  // namespace cmn
