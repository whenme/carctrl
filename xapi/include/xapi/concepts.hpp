// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <chrono>
#include <concepts>
#include <type_traits>

namespace cmn
{
/** @defgroup concepts Type attributes
 * @{
 */

/** Concept for scalar types
 */
template<typename Type>
concept Scalar = std::is_scalar_v<Type>;

/** Concept for integral types, excluding @c bool
 */
template<typename Type>
concept Integral = std::integral<Type> && !std::same_as<Type, bool>;

/** Concept for unsigned integral types, excluding @c bool and @c char
 *
 * @c char is not considered an unsigned integral type because it is not required to be unsigned, and is
 * a distinct type from @c unsigned @c char and @c signed @c char .
 */
template<typename Type>
concept UnsignedIntegral = std::unsigned_integral<Type> && !std::same_as<Type, bool> && !std::same_as<Type, char>;

/** Concept for arithmetic types, excluding @c bool
 */
template<typename Type>
concept Arithmetic = Integral<Type> || std::floating_point<Type>;

/** Concept for enumerations
 */
template<typename Type>
concept Enum = std::is_enum_v<Type>;

/** Concept for scoped enumerations
 *
 * @note The implementation can be simplified in C++23 by using std::is_scoped_enum_v
 */
template<typename Type>
concept ScopedEnum = magic_enum::is_scoped_enum_v<Type>;

/** Concept for rvalues
 *
 * @note This concept does not enforce that @c Type itself is an rvalue reference, but rather that @c Type&& may bind
 * only to rvalues.
 */
template<typename Type>
concept Rvalue = std::is_rvalue_reference_v<Type&&>;

/** Concept for standard layout types
 */
template<typename Type>
concept StandardLayoutType = std::is_standard_layout_v<Type>;

/** Concept for trivial standard layout types, commonly known as Plain Old Data (POD) types prior to C++20
 */
template<typename Type>
concept TrivialStandardLayoutType = StandardLayoutType<Type> && std::is_trivial_v<Type>;

/** Concept to detect if a type is the same as one of a list of types, with similar behavior to std::same_as
 * @note This concept returns false if an empty parameter pack is passed for Us, i.e. T is not the same as "nothing" */
template<typename T, typename... Us>
concept IsAnyOf = (std::same_as<T, Us> || ...);

/** Concept for durations
 */
template<typename Type>
concept Duration = std::same_as<Type, std::chrono::duration<typename Type::rep, typename Type::period>>;

/** Concept for invocable functions that return a value
 */
template<typename Function, typename ReturnType, typename... Args>
concept InvocableWithReturn = std::is_invocable_r_v<ReturnType, Function, Args...>;

/** @} */
}  // namespace cmn
