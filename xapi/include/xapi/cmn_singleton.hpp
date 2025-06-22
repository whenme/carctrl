// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cassert>
#include <memory>
#include <xapi/cmn_attribute.hpp>
#include <xapi/cmn_assert.hpp>

namespace cmn
{
/// @private
template<typename Type>
inline Type* g_singletonInstance = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/** Set a global pointer to a singleton instance
 *
 * This must be called within the constructor of a singleton class.
 *
 * @param instance is a pointer to the new instance value
 * @pre The pointer must be nullptr
 */
template<typename Type>
inline void setSingletonInstance(Type* instance) noexcept
{
    assert(g_singletonInstance<Type> == nullptr);
    g_singletonInstance<Type> = instance;
}

/** Reset a global instance pointer to nullptr
 *
 * This must be called within the destructor of a singleton class.
 *
 * @pre The pointer must point to a valid singleton instance
 */
template<typename Type>
inline void resetSingletonInstance() noexcept
{
    assert(g_singletonInstance<Type> != nullptr);
    g_singletonInstance<Type> = nullptr;
}

/** @return a pointer to a singleton instance
 */
template<typename Type>
[[nodiscard]] inline Type& getSingletonInstance() noexcept
{
    assert(g_singletonInstance<Type> != nullptr);
    return *g_singletonInstance<Type>;
}

/** class object for a singleton instance class
 *  constructor with variadic parameters
 */
template<typename T>
class Singleton
{
public:
    /**
     * @brief initialize singleton class object
     *  it will create object with constructor of variable parameters
     *
     * @param args variadic parameters of constructor
     * @return reference to the singleton instance
     */
    template<typename... Args>
    [[nodiscard]] static T& initInstance(Args&&... args)
    {
        if (s_m_instance == nullptr)
        {
            s_m_instance = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        return *s_m_instance;
    }

    /** @brief get singleton object instance
     * @return reference to the singleton instance */
    [[nodiscard]] static T& getInstance()
    {
        CMN_ASSERT(s_m_instance != nullptr);
        return *s_m_instance;
    }

    /** @brief remove default constructor */
    Singleton() = delete;

    /** @brief default destructor */
    virtual ~Singleton() = default;

    CMN_UNCOPYABLE_IMMOVABLE(Singleton)

private:
    static std::unique_ptr<T> s_m_instance;  ///< private object instance
};

/// @brief private instance
template<typename T>
std::unique_ptr<T> Singleton<T>::s_m_instance = nullptr;

}  //namespace cmn
