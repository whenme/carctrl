// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <xapi/assert.h>
#include <xapi/attributes.h>

#include <chrono>
#ifdef _MSC_VER
#include <time.h>
#else
#include <ctime>
#endif
#include <utility>

namespace cmn::chrono
{
/** @defgroup chrono Date and time manipulation utilities
 * @{
 */

/** Resynchronize system time after system timer recovery
 */
void resynchronizeSystemTime();

/** @return the current time, expressed in local time
 */
[[nodiscard]] inline std::tm getCurrentLocalTime()
{
    const auto now       = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm    localTime = {};
    // std::localtime is not thread-safe
#ifdef _MSC_VER
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif  // _MSC_VER
    return localTime;
}

/** @return the current calendar year
 */
[[nodiscard]] inline unsigned int getCurrentYear()
{
    constexpr auto baseYear  = 1900;
    const auto     localTime = getCurrentLocalTime();
    return static_cast<unsigned int>(baseYear + localTime.tm_year);
}

/** @return A predicate function returning @c true if the specified @c timeout has been reached, or @c false otherwise
 *
 * @tparam Clock type that fulfills the std 'Clock' requirements
 *
 * @param timeout Timeout duration
 */
template<typename Clock = std::chrono::steady_clock>
[[nodiscard]] inline auto makeTimeout(typename Clock::duration timeout)
{
    return [end = Clock::now() + timeout]() {
        return (Clock::now() >= end);
    };
}

/// Corresponding to std::chrono::nanoseconds but with 'double' Rep
using nanoseconds_float = std::chrono::duration<double, std::nano>;  // NOLINT(readability-identifier-naming)
/// Corresponding to std::chrono::microseconds but with 'double' Rep
using microseconds_float = std::chrono::duration<double, std::micro>;  // NOLINT(readability-identifier-naming)
/// Corresponding to std::chrono::milliseconds but with 'double' Rep
using milliseconds_float = std::chrono::duration<double, std::milli>;  // NOLINT(readability-identifier-naming)
/// Corresponding to std::chrono::seconds but with 'double' Rep
using seconds_float = std::chrono::duration<double>;  // NOLINT(readability-identifier-naming)

/** Chronometer class template providing utilities for measuring and reporting elapsed time
 * These family of classes can be thought of as a hand-held chronometer, where 'getElapsed()' is looking at the screen
 * @tparam Clock type that fulfills the std 'Clock' requirements, except that now() need not be static
 */
template<typename Clock>
class ChronometerImpl
{
public:
    /** Clock duration type
     */
    using ClockDuration = typename Clock::duration;

    /// Default constructor that does _not_ automatically start measuring
    ChronometerImpl() = default;

    /** Start the time measurement after construction/pause(), which can later be paused again
     * @note Calling @c start() more than once without @c pause() is an error and will trigger an assertion in debug
     * builds */
    void start() noexcept;

    /** Pauses the time measurement, which can later be resumed with 'start()'
     * @note Calling @c pause() more than once without @c start() is an error and will trigger an assertion in debug
     * builds */
    void pause() noexcept;

    /** Resets the time measurement back to zero, not affecting the running/paused state */
    void reset() noexcept;

    /** @tparam Duration type into which to cast the elapsed time, which internally is stored as Clock::duration
     * @return Total elapsed time between start/pause sections, including section between last start and present time
     *
     * @note This function is non-const because when using makeAutoStartChronometer a 'const' Chronometer can be
     * running, and different calls to getElapsed() maybe (almost certainly will) return different values, so it
     * appears as though the logical state of a Chronometer is modified, making it a non-const operation */
    template<typename Duration = std::chrono::milliseconds>
    [[nodiscard]] Duration getElapsed() noexcept;

private:
    bool                       m_paused{true};
    typename Clock::time_point m_start;
    ClockDuration              m_elapsed{0};

    Clock m_clock;
};

/** Chronometer using std::chrono::steady_clock for measurements in 'system' time */
using Chronometer = ChronometerImpl<std::chrono::steady_clock>;

/** Create a Chronometer object and start it, which on assignment can be used to 'reset' a Chronometer
 * @tparam Chronometer type to be created and started
 * @return Chronometer object that has been started
 * @note Eventually this function may need to be expanded to take arguments to be passed to the Chronometer constructor,
 * which some subclasses may need to construct m_clock (e.g. DeviceChronometer will need HardwareTimeTranslator
 * argument)
 */
template<typename Chronometer = Chronometer>
[[nodiscard]] Chronometer makeAutoStartChronometer() noexcept
{
    Chronometer chronometer;
    chronometer.start();
    return chronometer;
}

/** Times execution of a single function call
 *
 * @tparam Duration std::chrono::duration type to be returned
 * @tparam Chronometer type to be used for the measurement
 * @tparam Function callable type the execution of which will be measured
 * @tparam Args arguments to be forwarded to Function
 *
 * @param func Function to be called
 * @param args List of parameters to be forwarded to 'func'
 * @return Total time elapsed during execution of func(args...)
 */
template<typename Duration    = std::chrono::milliseconds,
         typename Chronometer = Chronometer,
         typename Function,
         typename... Args>
[[nodiscard]] inline Duration measureCall(Function&& func, Args&&... args)
{
    auto chronometer = makeAutoStartChronometer<Chronometer>();
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
    return chronometer.template getElapsed<Duration>();
}

/** @} */

/*
 * Implementation details follow
 */

template<typename Clock>
void ChronometerImpl<Clock>::start() noexcept
{
    CMN_ASSERT(m_paused && "Chronometer::start - cannot start if it's not paused");
    m_paused = false;
    m_start  = m_clock.now();
}

template<typename Clock>
void ChronometerImpl<Clock>::pause() noexcept
{
    CMN_ASSERT(!m_paused && "Chronometer::pause - cannot pause if it's already paused");
    m_paused = true;
    m_elapsed += (m_clock.now() - m_start);
}

template<typename Clock>
void ChronometerImpl<Clock>::reset() noexcept
{
    m_elapsed = ClockDuration{0};
    m_start   = m_clock.now();
}

template<typename Clock>
template<typename Duration>
Duration ChronometerImpl<Clock>::getElapsed() noexcept
{
    return std::chrono::duration_cast<Duration>(m_elapsed + (!m_paused ? (m_clock.now() - m_start) : Duration{0}));
}

}  // namespace cmn::chrono
