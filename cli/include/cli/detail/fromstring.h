// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_FROMSTRING_H_INCLUDED__
#define __CLI_DETAIL_FROMSTRING_H_INCLUDED__

#include <cassert>
#include <exception>
#include <limits>
#include <sstream>
#include <string>

namespace cli::detail
{
class BadConversion : public std::bad_cast
{
public:
    [[nodiscard]] const char* what() const noexcept override
    {
        return "bad fromString conversion: "
               "source string value could not be interpreted as target";
    }
};

template<typename T>
inline T fromString(const std::string& str);

template<>
inline std::string fromString(const std::string& str)
{
    return str;
}

template<>
inline std::nullptr_t fromString(const std::string& /*s*/)
{
    return nullptr;
}

namespace detail
{
template<typename T>
inline T unsignedDigitsFromString(const std::string& str)
{
    static constexpr int32_t k_decimal = 10;
    if (str.empty())
    {
        throw BadConversion();
    }
    T result = 0;
    for (const char& chr : str)
    {
        if (!std::isdigit(chr))
        {
            throw BadConversion();
        }
        const T digit = static_cast<T>(chr - '0');
        const T tmp   = static_cast<T>((result * k_decimal) + digit);
        if (result != ((tmp - digit) / k_decimal) || (tmp < result))
        {
            throw BadConversion();
        }
        result = tmp;
    }
    return result;
}

template<typename T>
inline T unsignedFromString(std::string str)
{
    if (str.empty())
    {
        throw BadConversion();
    }
    if (str[0] == '+')
    {
        str = str.substr(1);
    }
    return unsignedDigitsFromString<T>(str);
}

template<typename T>
inline T signedFromString(std::string str)
{
    if (str.empty())
    {
        throw BadConversion();
    }
    using U = std::make_unsigned_t<T>;
    if (str[0] == '-')
    {
        str         = str.substr(1);
        const U val = unsignedDigitsFromString<U>(str);
        if (val > static_cast<U>(-std::numeric_limits<T>::min()))
        {
            throw BadConversion();
        }
        return (-static_cast<T>(val));
    }

    if (str[0] == '+')
    {
        str = str.substr(1);
    }
    const U val = unsignedDigitsFromString<U>(str);
    if (val > static_cast<U>(std::numeric_limits<T>::max()))
    {
        throw BadConversion();
    }
    return static_cast<T>(val);
}
}  // namespace detail

template<>
inline signed char fromString(const std::string& str)
{
    return detail::signedFromString<signed char>(str);
}

template<>
inline short int fromString(const std::string& str)
{
    return detail::signedFromString<short int>(str);
}

template<>
inline int fromString(const std::string& str)
{
    return detail::signedFromString<int>(str);
}

template<>
inline long int fromString(const std::string& str)
{
    return detail::signedFromString<long int>(str);
}

template<>
inline long long int fromString(const std::string& str)
{
    return detail::signedFromString<long long int>(str);
}

template<>
inline unsigned char fromString(const std::string& str)
{
    return detail::unsignedFromString<unsigned char>(str);
}

template<>
inline unsigned short int fromString(const std::string& str)
{
    return detail::unsignedFromString<unsigned short int>(str);
}

template<>
inline unsigned int fromString(const std::string& str)
{
    return detail::unsignedFromString<unsigned int>(str);
}

template<>
inline unsigned long int fromString(const std::string& str)
{
    return detail::unsignedFromString<unsigned long int>(str);
}

template<>
inline unsigned long long int fromString(const std::string& str)
{
    return detail::unsignedFromString<unsigned long long int>(str);
}

// bool
template<>
inline bool fromString(const std::string& str)
{
    if (str == "true")
    {
        return true;
    }
    if (str == "false")
    {
        return false;
    }

    const auto value = detail::signedFromString<long long int>(str);
    if (value == 1)
    {
        return true;
    }
    if (value == 0)
    {
        return false;
    }
    throw BadConversion();
}

// chars
template<>
inline char fromString(const std::string& str)
{
    if (str.size() != 1)
    {
        throw BadConversion();
    }
    return str[0];
}

// floating points
template<>
inline float fromString(const std::string& str)
{
    if (std::any_of(str.begin(), str.end(), [](char ch) {
            return std::isspace(ch);
        }))
    {
        throw BadConversion();
    }
    std::string::size_type sz     = 0;
    float                  result = {};
    try
    {
        result = std::stof(str, &sz);
    }
    catch (const std::exception&)
    {
        throw BadConversion();
    }
    if (sz != str.size())
    {
        throw BadConversion();
    }
    return result;
}

template<>
inline double fromString(const std::string& str)
{
    if (std::any_of(str.begin(), str.end(), [](char ch) {
            return std::isspace(ch);
        }))
    {
        throw BadConversion();
    }

    std::string::size_type sz     = 0;
    double                 result = {};
    try
    {
        result = std::stod(str, &sz);
    }
    catch (const std::exception&)
    {
        throw BadConversion();
    }
    if (sz != str.size())
    {
        throw BadConversion();
    }
    return result;
}

template<>
inline long double fromString(const std::string& str)
{
    if (std::any_of(str.begin(), str.end(), [](char ch) {
            return std::isspace(ch);
        }))
    {
        throw BadConversion();
    }

    std::string::size_type sz     = 0;
    long double            result = {};
    try
    {
        result = std::stold(str, &sz);
    }
    catch (const std::exception&)
    {
        throw BadConversion();
    }
    if (sz != str.size())
    {
        throw BadConversion();
    }
    return result;
}

template<typename T>
inline T fromString(const std::string& str)
{
    std::stringstream interpreter;
    T                 result;

    if (!(interpreter << str) || !(interpreter >> result) || !(interpreter >> std::ws).eof())
    {
        throw BadConversion();
    }

    return result;
}

inline std::string commonPrefix(const std::vector<std::string>& vec)
{
    assert(!vec.empty());
    std::string prefix;

    // find the shorter string
    auto smin = std::min_element(vec.begin(), vec.end(), [](const std::string& s1, const std::string& s2) {
        return s1.size() < s2.size();
    });

    for (std::size_t i = 0; i < smin->size(); ++i)
    {
        // check if i-th element is equal in each input string
        const char ch = (*smin)[i];
        for (const auto& val : vec)
        {
            if (val[i] != ch)
            {
                return prefix;
            }
        }
        prefix += ch;
    }

    return prefix;
}

}  // namespace cli::detail

#endif  // __CLI_DETAIL_FROMSTRING_H_INCLUDED__
