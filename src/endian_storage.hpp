#pragma once
#include <cstddef>
#include <cstdint>

#include <bit>
#include <array>
#include <span>
#include <algorithm>
#include <ranges>
#include <utility>

#include "concepts.hpp"

namespace vmgs {
    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    [[nodiscard]]
    constexpr Ty endian_load(const ByteTy (&buf)[sizeof(Ty)]) noexcept {
        if constexpr (Endian == std::endian::native) {
            return std::bit_cast<Ty>(buf);
        } else {
            if constexpr (std::is_integral<Ty>{}) {
                return std::byteswap(std::bit_cast<Ty>(buf));
            } else {    // should be floating-point type, where `std::byteswap` cannot be invoked
                using SameSizedIntegerTy =
                    std::conditional_t<sizeof(Ty) == 4, uint32_t,                // for float
                        std::conditional_t<sizeof(Ty) == 8, uint64_t, void>>;    // for double
                return std::bit_cast<Ty>(std::byteswap(std::bit_cast<SameSizedIntegerTy>(buf)));
            }
        }
    }

    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    [[nodiscard]]
    constexpr Ty endian_load(const std::array<ByteTy, sizeof(Ty)>& buf) noexcept {
        if constexpr (Endian == std::endian::native) {
            return std::bit_cast<Ty>(buf);
        } else {
            if constexpr (std::is_integral<Ty>{}) {
                return std::byteswap(std::bit_cast<Ty>(buf));
            } else {    // should be floating-point type, where `std::byteswap` cannot be invoked
                using SameSizedIntegerTy =
                    std::conditional_t<sizeof(Ty) == 4, uint32_t,                // for float
                        std::conditional_t<sizeof(Ty) == 8, uint64_t, void>>;    // for double
                return std::bit_cast<Ty>(std::byteswap(std::bit_cast<SameSizedIntegerTy>(buf)));
            }
        }
    }

    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    [[nodiscard]]
    constexpr Ty endian_load(std::span<const ByteTy, sizeof(Ty)> buf) noexcept {
        std::array<ByteTy, sizeof(Ty)> data;
        std::ranges::copy(buf, data.begin());
        return endian_load<Ty, Endian, ByteTy>(data);
    }

    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    constexpr void endian_store(ByteTy (&buf)[sizeof(Ty)], Ty val) noexcept {
        if constexpr (Endian == std::endian::native) {
            std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(val), std::begin(buf));
        } else {
            if constexpr (std::is_integral<Ty>{}) {
                std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(val)), std::begin(buf));
            } else {    // should be floating-point type, where `std::byteswap` cannot be invoked
                using SameSizedIntegerTy =
                    std::conditional_t<sizeof(Ty) == 4, uint32_t,               // for float
                        std::conditional_t<sizeof(Ty) == 8, uint64_t, void>>;   // for double
                std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(std::bit_cast<SameSizedIntegerTy>(val))), std::begin(buf));
            }
        }
    }

    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    constexpr void endian_store(std::array<ByteTy, sizeof(Ty)>& buf, Ty val) noexcept {
        if constexpr (Endian == std::endian::native) {
            buf = std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(val);
        } else {
            if constexpr (std::is_integral<Ty>{}) {
                buf = std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(val));
            } else {    // should be floating-point type, where `std::byteswap` cannot be invoked
                using SameSizedIntegerTy =
                    std::conditional_t<sizeof(Ty) == 4, uint32_t,               // for float
                        std::conditional_t<sizeof(Ty) == 8, uint64_t, void>>;   // for double
                buf = std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(std::bit_cast<SameSizedIntegerTy>(val)));
            }
        }
    }

    template<concepts::arithmetic Ty, std::endian Endian = std::endian::native, concepts::byte_compatible ByteTy>
    constexpr void endian_store(std::span<ByteTy, sizeof(Ty)> buf, Ty val) noexcept {
        if constexpr (Endian == std::endian::native) {
            std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(val), buf.begin());;
        } else {
            if constexpr (std::is_integral<Ty>{}) {
                std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(val)), buf.begin());
            } else {    // should be floating-point type, where `std::byteswap` cannot be invoked
                using SameSizedIntegerTy =
                    std::conditional_t<sizeof(Ty) == 4, uint32_t,               // for float
                        std::conditional_t<sizeof(Ty) == 8, uint64_t, void>>;   // for double
                std::ranges::copy(std::bit_cast<std::array<ByteTy, sizeof(Ty)>>(std::byteswap(std::bit_cast<SameSizedIntegerTy>(val))), buf.begin());
            }
        }
    }
}
