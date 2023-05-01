#pragma once
#include <cstddef>
#include <cstdint>

#include <array>
#include <span>

namespace vmgs {
    [[nodiscard]]
    uint32_t crc32_iso3309(uint32_t initial, const void* data, size_t size) noexcept;

    template<typename Ty, std::size_t Cnt>
    [[nodiscard]]
    uint32_t crc32_iso3309(uint32_t initial, const std::array<Ty, Cnt>& data) noexcept {
        return crc32_iso3309(initial, data.data(), Cnt * sizeof(Ty));
    }

    template<typename Ty, std::size_t Extent>
    [[nodiscard]]
    uint32_t crc32_iso3309(uint32_t initial, std::span<Ty, Extent> data) noexcept {
        return crc32_iso3309(initial, data.data(), data.size_bytes());
    }
}
