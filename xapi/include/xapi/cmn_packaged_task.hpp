// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <xapi/cmn_assert.h>
#include <xapi/cmn_concepts.h>
#include <xapi/cmn_move_only_function.h>

#include <concepts>
#include <exception>
#include <functional>
#include <future>
#include <optional>
#include <type_traits>
#include <utility>

namespace cmn
{
/** @addtogroup containers
 * @{
 */

/** @private
 */
template<typename...>
class PackagedTask;

/** Polymorphic move-only function wrapper that can be invoked asynchronously
 *
 * @todo Replace with std::packaged_task when the MSVC implementation supports storing a move-only callable object.
 * See issue #321 in the microsoft/STL project:
 *
 * @verbatim
 *     <future> : packaged_task can't be constructed from a move-only lambda
 * @endverbatim
 */
template<typename ReturnType, typename... Args>
class PackagedTask<ReturnType(Args...)>
{
public:
    /** Construct an empty packaged task
     */
    PackagedTask() = default;

    /** Construct a move-only function from a callable object
     *
     * @param func Callable object to store
     */
    template<cmn::InvocableWithReturn<ReturnType, Args...> Function>
        requires(!std::same_as<std::remove_cvref_t<Function>, PackagedTask<ReturnType(Args...)>>)
    // The template is constrained so that it doesn't hide the move constructor
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    explicit PackagedTask(Function&& func) : m_func{std::forward<Function>(func)}
    {
    }

    ~PackagedTask() = default;

    CMN_DEFAULT_MOVABLE(PackagedTask)

    CMN_UNCOPYABLE(PackagedTask)

    /** @return The future associated with the promise result
     */
    [[nodiscard]] std::future<ReturnType> getFuture()
    {
        return m_promise.get_future();
    }

    /** Call the target function and set the result in the promise
     *
     * @param args Arguments to pass to the function
     */
    void operator()(Args... args)
    {
        CMN_ASSERT(m_func && "Packaged task called without a target");
        if constexpr (std::is_same_v<ReturnType, void>)
        {
            std::exception_ptr exception;
            try
            {
                std::invoke(m_func, std::forward<Args>(args)...);
            }
            catch (...)
            {
                exception = std::current_exception();
            }

            if (exception)
            {
                m_promise.set_exception(exception);
            }
            else
            {
                m_promise.set_value();
            }
        }
        else
        {
            std::optional<ReturnType> result;
            try
            {
                result = std::invoke(m_func, std::forward<Args>(args)...);
            }
            catch (...)
            {
                m_promise.set_exception(std::current_exception());
            }

            if (result.has_value())
            {
                m_promise.set_value(std::move(*result));
            }
        }
    }

private:
    MoveOnlyFunction<ReturnType(Args...)> m_func;
    std::promise<ReturnType>              m_promise;
};

/** @} */
}  // namespace cmn

