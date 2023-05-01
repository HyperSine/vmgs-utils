#pragma once
#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <ranges>
#include <vector>
#include <utility>
#include <format>
#include <stdexcept>

#include "IBlockDevice.hpp"

namespace vmgs {
    constexpr uint32_t GPT_REVISION = 0x00010000;

    constexpr std::array<std::byte, 8> GPT_SIGNATURE = 
        { std::byte{'E'}, std::byte{'F'}, std::byte{'I'}, std::byte{' '},
          std::byte{'P'}, std::byte{'A'}, std::byte{'R'}, std::byte{'T'} };

    enum class GptLba : uint64_t {};

    struct GptGuid {
        uint32_t data1;
        uint16_t data2;
        uint16_t data3;
        std::array<uint8_t, 8> data4;

        [[nodiscard]]
        friend bool operator==(const GptGuid& lhs, const GptGuid& rhs) noexcept = default;

        [[nodiscard]]
        friend bool operator!=(const GptGuid& lhs, const GptGuid& rhs) noexcept = default;
    };

    static_assert(sizeof(GptGuid) == 0x10);

    class GptHeader {
        friend class Gpt;
        friend struct GptHeaderLayout;
    private:
        GptLba m_current_lba;
        GptLba m_backup_lba;
        GptLba m_first_usable_lba;
        GptLba m_last_usable_lba;
        GptGuid m_guid;
        GptLba m_partition_entries_lba;
        uint32_t m_partition_entries_num;
        uint32_t m_partition_entries_checksum;

    public:
        [[nodiscard]]
        GptLba current_lba() const noexcept {
            return m_current_lba;
        }

        [[nodiscard]]
        GptLba backup_lba() const noexcept {
            return m_backup_lba;
        }

        [[nodiscard]]
        GptLba first_usable_lba() const noexcept {
            return m_first_usable_lba;
        }

        [[nodiscard]]
        GptLba last_usable_lba() const noexcept {
            return m_last_usable_lba;
        }

        [[nodiscard]]
        const GptGuid& guid() const noexcept {
            return m_guid;
        }

        [[nodiscard]]
        GptLba partition_entries_lba() const noexcept {
            return m_partition_entries_lba;
        }

        [[nodiscard]]
        uint32_t partition_entries_num() const noexcept {
            return m_partition_entries_num;
        }

        [[nodiscard]]
        lclosed_interval<uint64_t> partition_entries_lba_range(size_t block_size) const noexcept;

        [[nodiscard]]
        size_t partition_entries_size() const noexcept;
    };

    struct GptPartitionAttributes {
        uint64_t platform_required : 1;
        uint64_t should_ignore : 1;
        uint64_t legacy_bios_bootable : 1;
        uint64_t reserved : 45;
        uint64_t partition_specified : 16;
    };

    class GptPartitionEntry {
        friend class Gpt;
        friend struct GptPartitionEntryLayout;
    private:
        GptGuid m_type_guid;
        GptGuid m_unique_guid;
        GptLba m_first_lba;
        GptLba m_last_lba;
        GptPartitionAttributes m_attributes;
        std::array<char16_t, 36> m_name;

    public:
        [[nodiscard]]
        const GptGuid& type_guid() const noexcept {
            return m_type_guid;
        }

        [[nodiscard]]
        const GptGuid& unique_guid() const noexcept {
            return m_unique_guid;
        }

        [[nodiscard]]
        GptLba first_lba() const noexcept {
            return m_first_lba;
        }

        [[nodiscard]]
        GptLba last_lba() const noexcept {
            return m_last_lba;
        }

        [[nodiscard]]
        GptPartitionAttributes attributes() const noexcept {
            return m_attributes;
        }

        [[nodiscard]]
        std::u16string_view name() const noexcept {
            return std::u16string_view{
                m_name.data(), static_cast<size_t>(std::ranges::find(m_name, char16_t{}) - m_name.begin())
            };
        }

        [[nodiscard]]
        lclosed_interval<uint64_t> lba_range() const noexcept {
            return lclosed_interval<uint64_t>{
                .min = std::to_underlying(m_first_lba), .max = std::to_underlying(m_last_lba) + 1
            };
        }
    };

    class Gpt {
    private:
        GptHeader m_header;
        std::vector<GptPartitionEntry> m_partitions;

        Gpt(GptHeader&& header, std::vector<GptPartitionEntry>&& partitions) noexcept
            : m_header{ std::move(header) }, m_partitions{ std::move(partitions) } {}

    public:
        [[nodiscard]]
        const GptHeader& header() const noexcept {
            return m_header;
        }

        [[nodiscard]]
        const std::vector<GptPartitionEntry>& partitions() const noexcept {
            return m_partitions;
        }

        [[nodiscard]]
        static Gpt load_from(IBlockDevice& block_device);
    };

    struct GptLbaOutOfRangeError : std::out_of_range {
        using std::out_of_range::out_of_range;
    };

    struct GptInvalidSignatureError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct GptChecksumValidationError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

template<>
struct std::formatter<vmgs::GptGuid, char> {
    bool uppercase;

    template<typename ParserContextTy>
    constexpr auto parse(ParserContextTy& parser_ctx) {
        auto iter = parser_ctx.begin();

        if (iter != parser_ctx.end()) {
            switch (*iter) {
                case 'x':
                    uppercase = false;
                    ++iter;
                    break;
                case 'X':
                    uppercase = true;
                    ++iter;
                    break;
                default:
                    break;
            }
        }

        if (iter != parser_ctx.end() && *iter != '}') {
            throw format_error(::std::format("Unexpected format spec: `{}`.", *iter));
        }

        return iter;
    }

    template<typename FormatContextTy>
    auto format(const vmgs::GptGuid& guid, FormatContextTy& format_ctx) const {
        if (uppercase) {
            return format_to(
                format_ctx.out(),
                "{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
                guid.data1,
                guid.data2,
                guid.data3,
                guid.data4[0], guid.data4[1],
                guid.data4[2], guid.data4[3], guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7]
            );
        } else {
            return format_to(
                format_ctx.out(),
                "{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                guid.data1,
                guid.data2,
                guid.data3,
                guid.data4[0], guid.data4[1],
                guid.data4[2], guid.data4[3], guid.data4[4], guid.data4[5], guid.data4[6], guid.data4[7]
            );
        }
    }
};
