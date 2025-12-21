// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cmn/assert.hpp>
#include <cmn/attributes.hpp>
#include <cmn/concepts.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <utility>

namespace cmn
{
/** @addtogroup containers
 * @{
 */

/** @private
 */
template<typename...>
class MoveOnlyFunction;

/** General-purpose polymorphic move-only function wrapper
 *
 * @todo Replace with std::move_only_function when C++23 is available
 */
template<typename ReturnType, typename... Args>
class MoveOnlyFunction<ReturnType(Args...)>
{
public:
    /** Construct an empty move-only function
     */
    MoveOnlyFunction() = default;

    /** Construct a move-only function from a callable object
     *
     * @param func Callable object to store
     */
    template<cmn::InvocableWithReturn<ReturnType, Args...> Function>
        requires(!std::same_as<std::remove_cvref_t<Function>, MoveOnlyFunction<ReturnType(Args...)>>)
    // The template is constrained so that it doesn't hide the move constructor
    // Implicit conversions are intentionally allowed
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload, google-explicit-constructor)
    MoveOnlyFunction(Function&& func) : m_func{std::make_unique<Shelf<Function>>(std::forward<Function>(func))}
    {
    }

    ~MoveOnlyFunction() = default;

    CMN_DEFAULT_MOVABLE(MoveOnlyFunction)

    CMN_UNCOPYABLE(MoveOnlyFunction)

    /** @return @c true if the move-only function has a target, @c false otherwise
     */
    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_func);
    }

    /** Call the target function
     *
     * @param args Arguments to pass to the function
     *
     * @return Return value of the function
     */
    ReturnType operator()(Args... args)
    {
        CMN_ASSERT(m_func && "Move-only function called without a target");
        return m_func->call(std::forward<Args>(args)...);
    }

private:
    class Base
    {
    public:
        virtual ~Base() = default;

        virtual ReturnType call(Args... args) = 0;

        CMN_UNCOPYABLE_IMMOVABLE(Base)

    protected:
        Base() = default;
    };

    template<typename Function>
    class Shelf final : public Base
    {
    public:
        // func is forwarded, not moved, because func is forwarded to the Shelf constructor from the MoveOnlyFunction
        // constructor, and func is a forwarding reference in the MoveOnlyFunction constructor
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        explicit Shelf(Function&& func) : m_func{std::forward<Function>(func)}
        {
        }

        ReturnType call(Args... args) override
        {
            return std::invoke(m_func, std::forward<Args>(args)...);
        }

    private:
        Function m_func;
    };

    std::unique_ptr<Base> m_func;
};

/** @} */
}  // namespace cmn
