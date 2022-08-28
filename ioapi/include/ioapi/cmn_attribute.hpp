// SPDX-License-Identifier: GPL-2.0

#ifndef __CMN_ATTRIBUTE_HPP__
#define __CMN_ATTRIBUTE_HPP__

#include <type_traits>
#include <utility>

/** Declare that objects of the specified class are not copyable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_UNCOPYABLE(ClassName)                                                             \
    ClassName(const ClassName&)            = delete; /* NOLINT(bugprone-macro-parentheses) */ \
    ClassName& operator=(const ClassName&) = delete; /* NOLINT(bugprone-macro-parentheses) */

/** Declare that objects of the specified class are not movable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-macro-parentheses)
#define CMN_IMMOVABLE(ClassName)                                                         \
    ClassName(ClassName&&)            = delete; /* NOLINT(bugprone-macro-parentheses) */ \
    ClassName& operator=(ClassName&&) = delete; /* NOLINT(bugprone-macro-parentheses) */

/** Declare that objects of the specified class are neither copyable nor movable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_UNCOPYABLE_IMMOVABLE(ClassName) \
    CMN_UNCOPYABLE(ClassName)               \
    CMN_IMMOVABLE(ClassName)

/** Declare that objects of the specified class are default-copyable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-macro-parentheses)
#define CMN_DEFAULT_COPYABLE(ClassName)                                                        \
    ClassName(const ClassName&)            = default; /* NOLINT(bugprone-macro-parentheses) */ \
    ClassName& operator=(const ClassName&) = default; /* NOLINT(bugprone-macro-parentheses) */

/** Declare that objects of the specified class are default-movable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, bugprone-macro-parentheses)
#define CMN_DEFAULT_MOVABLE(ClassName)                                                             \
    ClassName(ClassName&&) noexcept            = default; /* NOLINT(bugprone-macro-parentheses) */ \
    ClassName& operator=(ClassName&&) noexcept = default; /* NOLINT(bugprone-macro-parentheses) */

/** Declare that objects of the specified class are default-copyable and movable
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_DEFAULT_COPYABLE_MOVABLE(ClassName) \
    CMN_DEFAULT_COPYABLE(ClassName)             \
    CMN_DEFAULT_MOVABLE(ClassName)

/** Declare that the specified class is a pure interface
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_PURE_INTERFACE(ClassName) TNG_UNCOPYABLE_IMMOVABLE(ClassName)

/** Define a boolean constant that is @c true when the specified type contains the specified nested type, or @c false
 * otherwise
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_DETECT_NESTED_TYPE_PRESENCE(TypeName, NestedTypeName, ConstantName) \
    static constexpr bool ConstantName = requires                               \
    {                                                                           \
        typename TypeName::NestedTypeName;                                      \
    }

/** Define a boolean constant that is @c true when the specified type contains the specified member, or
 *  @c false otherwise
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_DETECT_MEMBER_VARIABLE_PRESENCE(TypeName, memberName, ConstantName) \
    static constexpr bool ConstantName = requires                               \
    {                                                                           \
        TypeName::memberName;                                                   \
    }

/** Define a boolean constant that is @c true when the specified type contains the specified member function, or
 *  @c false otherwise
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CMN_DETECT_MEMBER_FUNCTION_PRESENCE(TypeName, functionName, ConstantName) \
    static constexpr bool ConstantName = requires                                 \
    {                                                                             \
        std::declval<TypeName>().functionName();                                  \
    }

#endif  // __CMN_ATTRIBUTE_HPP__

