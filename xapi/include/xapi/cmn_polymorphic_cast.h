// SPDX-License-Identifier: GPL-2.0

#pragma once

#include <xapi/cmn_assert.h>

#include <concepts>
#include <type_traits>

namespace cmn
{
/** Downcast a pointer to a polymorphic object safely and efficiently
 *
 * The built-in static_cast can be used for efficiently downcasting pointers to polymorphic objects, but provides no
 * error detection for the case where the pointer being cast actually points to the wrong derived class. This function
 * retains the efficiency of static_cast for non-debug builds, but for debug builds adds safety via an
 * assertion check that a dynamic_cast succeeds.
 *
 * @param ptr is the pointer to the object, with the base type
 * @return a pointer to the object, with the derived type
 */
template<class DerivedPtr, class Base>
    requires std::is_pointer_v<DerivedPtr> && std::derived_from<std::remove_pointer_t<DerivedPtr>, Base>
[[nodiscard]] inline DerivedPtr polymorphicDowncast(Base* ptr) noexcept
{
    TNG_ASSERT(dynamic_cast<DerivedPtr>(ptr) == ptr);
    return static_cast<DerivedPtr>(ptr);
}

/** Downcast a reference to a polymorphic object safely and efficiently
 *
 * The built-in static_cast can be used for efficiently downcasting references to polymorphic objects, but provides no
 * error detection for the case where the reference being cast actually refers to the wrong derived class. This function
 * retains the efficiency of static_cast for non-debug builds, but for debug builds adds safety via an
 * assertion check that a dynamic_cast succeeds.
 *
 * @param ref is the reference to the object, with the base type
 * @return a reference to the object, with the derived type
 */
template<class DerivedRef, class Base>
    requires std::is_reference_v<DerivedRef> && std::derived_from<std::remove_reference_t<DerivedRef>, Base>
[[nodiscard]] inline DerivedRef polymorphicDowncast(Base& ref) noexcept
{
#ifndef NDEBUG
    using DerivedType = std::remove_reference_t<DerivedRef>;
#endif
    TNG_ASSERT(dynamic_cast<DerivedType*>(&ref) == &ref);
    return static_cast<DerivedRef>(ref);
}

}  // namespace cmn
