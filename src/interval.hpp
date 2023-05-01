#pragma once
#include <functional>
#include "concepts.hpp"

namespace vmgs {
    template<typename Ty, bool LeftInclusive, bool RightInclusive>
    struct interval {
        Ty min;
        Ty max;

        [[nodiscard]]
        constexpr Ty length() const noexcept
            requires(std::integral<Ty> && !LeftInclusive && !RightInclusive)
        {
            return empty() ? 0 : (max - min - 1);
        }

        [[nodiscard]]
        constexpr Ty length() const noexcept
            requires(std::integral<Ty> && (LeftInclusive && !RightInclusive || !LeftInclusive && RightInclusive))
        {
            return empty() ? 0 : (max - min);
        }

        [[nodiscard]]
        constexpr Ty length() const noexcept
            requires(std::integral<Ty> && LeftInclusive && RightInclusive)
        {
            return empty() ? 0 : (max - min + 1);
        }

        [[nodiscard]]
        constexpr bool empty() const noexcept
            requires(std::integral<Ty> && !LeftInclusive && !RightInclusive)
        {
            return min >= max || max - min == 1;
        }

        [[nodiscard]]
        constexpr bool empty() const noexcept
            requires(std::integral<Ty> && (LeftInclusive && !RightInclusive || !LeftInclusive && RightInclusive))
        {
            return min >= max;
        }

        [[nodiscard]]
        constexpr bool empty() const noexcept
            requires(std::integral<Ty> && LeftInclusive && RightInclusive)
        {
            return min > max;
        }

        [[nodiscard]]
        constexpr bool contains(Ty value) const noexcept {
            using left_comparer = std::conditional_t<LeftInclusive, std::less_equal<Ty>, std::less<Ty>>;
            using right_comparer = std::conditional_t<RightInclusive, std::less_equal<Ty>, std::less<Ty>>;
            return left_comparer{}(min, value) && right_comparer{}(value, max);
        }
    };

    template<typename Ty>
    using open_interval = interval<Ty, false, false>;

    template<typename Ty>
    using closed_interval = interval<Ty, true, true>;

    template<typename Ty>
    using lclosed_interval = interval<Ty, true, false>;

    template<typename Ty>
    using rclosed_interval = interval<Ty, true, false>;
}
