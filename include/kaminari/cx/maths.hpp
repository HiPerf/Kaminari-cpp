#pragma once

#include <inttypes.h>
#include <limits>
#include <type_traits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>


namespace cx
{
    template <typename T>
    constexpr T abs(const T x)
    {
        return x >= 0 ? x : -x;
    }

    template <typename T>
    constexpr bool feq(T x, T y)
    {
        return abs(x - y) <= std::numeric_limits<T>::epsilon();
    }

    template <typename T>
    constexpr T max(const T x, const T y)
    {
        return x > y ? x : y;
    }

    namespace detail
    {
        template <typename T>
        constexpr float sqrt(const T x, const T guess)
        {
            return feq(guess, (guess + x / guess) / T{2}) ? guess :
                sqrt(x, (guess + x / guess) / T{2});
        }
    }

    constexpr float sqrt(const int32_t x)
    {
        return x == 0 ? 0.0f : detail::sqrt<float>(static_cast<float>(x), static_cast<float>(x));
    }

    template <typename T>
    constexpr float sqrt(const T x)
    {
        return feq(x, T{0}) ? T{0} : detail::sqrt(x, x);
    }

    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    constexpr bool eq(T x, T y)
    {
        return x == y;
    }

    template<class T, typename std::enable_if<!std::is_integral<T>::value, int>::type = 0>
    constexpr bool eq(T x, T y)
    {
        return feq(x, y);
    }
    
    template <typename T>
    constexpr inline T sign(T x)
    {
        if (x < 0) return T(-1);
        if (x > 0) return T(1);
        return T(0);
    }

    template <typename T>
    constexpr inline T is_negative(T x)
    {
        if (x < 0) return T(-1);
        return T(0);
    }

    template <typename T, typename Va, typename Vb>
    constexpr inline T distance2(Va&& a, Vb&& b)
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return glm::distance2(a, b);
        }

        return glm::compAdd(a * b);
    }

    template <typename T, typename Va, typename Vb>
    constexpr inline T manhattan(Va&& a, Vb&& b)
    {
        return glm::compAdd(glm::max(a, b) - glm::min(a, b));
    }

    template <>
    constexpr inline uint32_t manhattan(const glm::tvec3<uint32_t, glm::highp>& a, const glm::tvec3<uint32_t, glm::highp>& b)
    {
        return abs(a.x - b.x) + abs(a.z - b.z);
    }

    template <>
    constexpr inline uint32_t manhattan(const glm::tvec2<uint32_t, glm::highp>& a, const glm::tvec2<uint32_t, glm::highp>& b)
    {
        return abs(a.x - b.x) + abs(a.y - b.y);
    }
}
