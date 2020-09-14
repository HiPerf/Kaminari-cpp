#pragma once

#include <limits>
#include <inttypes.h>


namespace cx
{
    namespace overflow
    {
        template <typename T>
        inline constexpr bool le(T x, T y, T threshold = std::numeric_limits<T>::max() / 2)
        {
            return 
                (x <= y && (y - x) < threshold) ||  // Standard case, 200 < 255 && (255 - 200) < thr
                (x > y && (x - y) > threshold);    // Overflow case, 255 > 1 && (255 - 1) > thr
        }

        template <typename T>
        inline constexpr bool ge(T x, T y, T threshold = std::numeric_limits<T>::max() / 2)
        {
            return 
                (x >= y && (x - y) < threshold) ||  // Standard case, 255 > 200 && (255 - 200) < thr
                (y > x && (y - x) > threshold);    // Overflow case, 255 > 1 && (255 - 1) > thr
        }

        template <typename T>
        inline constexpr T sub(T x, T y)
        {
            return 
                (x >= y) ? static_cast<T>(x - y) : static_cast<T>(std::numeric_limits<T>::max() - y + x);
        }

        template <typename T>
        inline constexpr T sub0(T x, T y)
        {
            if (auto z = sub(x, y); z != 0)
            {
                return z;
            }
            return std::numeric_limits<T>::max();
        }

        template <typename T>
        inline constexpr T inc(T x)
        {
            return static_cast<T>(x + 1);
        }

        template <typename T>
        inline constexpr T inc0(T x)
        {
            if (auto z = inc(x); z != 0)
            {
                return z;
            }
            return 1;
        }
    }
}
