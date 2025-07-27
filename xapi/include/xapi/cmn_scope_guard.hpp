// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <types/attributes.h>

#include <functional>
#include <type_traits>
#include <utility>

namespace cmn
{
/** General implementation of the "Resource Acquisition Is Initialization" (RAII) idiom
 *
 * A scope guard ensures that a function is executed upon leaving the
 * current scope unless otherwise told.
 *
 * This code is based on Andrei Alexandrescu's presentation "Systematic Error
 * Handling in C++" from "C++ and Beyond 2012", which is in the public domain.
 *
 * @tparam Callable A callable object such as a lambda function, a functor, or a void(*)()
 * function pointer marked as noexcept
 *
 * @note std::function<void()> cannot be used since it is not possible to mark it as
 * noexcept.
 */
template<typename Callable>
class ScopeGuard
{
public:
    static_assert(std::is_nothrow_invocable_v<Callable>);

    /** Construct a scope guard
     *
     * @param callable is the function-like object that is scheduled to be invoked
     */
    explicit ScopeGuard(Callable callable) : m_callable{std::move(callable)}
    {
    }

    /** Execute the function unless the scope guard has been dismissed
     */
    ~ScopeGuard()
    {
        if (m_active)
        {
            std::invoke(m_callable);
        }
    }

    ScopeGuard() = delete;

    TNG_UNCOPYABLE_IMMOVABLE(ScopeGuard)

    /** Dismiss the scope guard so the callable will _not_ be invoked when the guard is destructed.
     */
    void dismiss() noexcept
    {
        m_active = false;
    }

private:
    Callable m_callable;
    bool     m_active = true;
};

}  // namespace cmn
