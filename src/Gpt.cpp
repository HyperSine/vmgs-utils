#include "Gpt.hpp"
#include "endian_storage.hpp"
#include "crc32.hpp"

namespace vmgs {
    struct GptLbaLayout;
    struct GptGuidLayout;
    struct GptHeaderLayout;
    struct GptPartitionAttributesLayout;
    struct GptPartitionEntryLayout;

    struct GptLbaLayout {
        std::array<std::byte, 8> value;

        [[nodiscard]]
        GptLba load(lclosed_interval<uint64_t> lba_range) const;

        void store(const GptLba& lba) noexcept;
    };

    static_assert(sizeof(GptLbaLayout) == 0x8);
    static_assert(alignof(GptLbaLayout) == alignof(std::byte));

    struct GptGuidLayout {
        std::array<std::byte, 4> data1;
        std::array<std::byte, 2> data2;
        std::array<std::byte, 2> data3;
        std::array<std::byte, 8> data4;

        [[nodiscard]]
        GptGuid load() const noexcept;

        void store(const GptGuid& guid) noexcept;
    };

    static_assert(sizeof(GptGuidLayout) == 0x10);
    static_assert(alignof(GptGuidLayout) == alignof(std::byte));

    struct GptHeaderLayout {
        std::array<std::byte, 8> signature;
        std::array<std::byte, 4> revision;
        std::array<std::byte, 4> header_size;
        std::array<std::byte, 4> header_checksum;
        std::array<std::byte, 4> reserved_zero;
        GptLbaLayout current_lba;
        GptLbaLayout backup_lba;
        GptLbaLayout first_usable_lba;
        GptLbaLayout last_usable_lba;
        GptGuidLayout guid;
        GptLbaLayout partition_entries_lba;
        std::array<std::byte, 4> partition_entries_num;
        std::array<std::byte, 4> partition_entry_size;
        std::array<std::byte, 4> partition_entries_checksum;

        [[nodiscard]]
        GptHeader load(lclosed_interval<uint64_t> lba_range, size_t block_size) const;

        void store(const GptHeader& header) noexcept;   // todo
    };

    static_assert(sizeof(GptHeaderLayout) == 0x5c);
    static_assert(alignof(GptHeaderLayout) == alignof(std::byte));

    struct GptPartitionAttributesLayout {
        std::array<uint8_t, 8> value;

        [[nodiscard]]
        GptPartitionAttributes load() const;

        void store(const GptPartitionAttributes& attributes) noexcept;
    };

    static_assert(sizeof(GptPartitionAttributesLayout) == 0x8);
    static_assert(alignof(GptPartitionAttributesLayout) == alignof(std::byte));

    struct GptPartitionEntryLayout {
        GptGuidLayout type_guid;
        GptGuidLayout unique_guid;
        GptLbaLayout first_lba;
        GptLbaLayout last_lba;
        GptPartitionAttributesLayout attributes;
        std::array<uint8_t, 72> name;

        [[nodiscard]]
        GptPartitionEntry load(lclosed_interval<uint64_t> lba_range) const;

        void store(const GptPartitionEntry& entry) noexcept;   // todo
    };

    static_assert(sizeof(GptPartitionEntryLayout) == 0x80);
    static_assert(alignof(GptPartitionEntryLayout) == alignof(std::byte));

    GptLba GptLbaLayout::load(lclosed_interval<uint64_t> lba_range) const {
        auto v = endian_load<uint64_t, std::endian::little>(value);
        if (lba_range.contains(v)) {
            return GptLba{ v };
        } else {
            throw GptLbaOutOfRangeError(std::format("Bad GPT LBA: 0x{:x} is not in range [0x{:x}, 0x{:x}).", v, lba_range.min, lba_range.max));
        }
    }

    void GptLbaLayout::store(const GptLba& lba) noexcept {
        endian_store<uint64_t, std::endian::little>(value, std::to_underlying(lba));
    }

    GptGuid GptGuidLayout::load() const noexcept {
        GptGuid retval;

        retval.data1 = endian_load<uint32_t, std::endian::little>(data1);
        retval.data2 = endian_load<uint16_t, std::endian::little>(data2);
        retval.data3 = endian_load<uint16_t, std::endian::little>(data3);
        std::ranges::transform(data4, retval.data4.begin(), std::to_integer<uint8_t>);

        return retval;
    }

    void GptGuidLayout::store(const GptGuid& guid) noexcept {
        endian_store<uint32_t, std::endian::little>(data1, guid.data1);
        endian_store<uint16_t, std::endian::little>(data2, guid.data2);
        endian_store<uint16_t, std::endian::little>(data3, guid.data3);
        std::ranges::transform(guid.data4, data4.begin(), [](auto v) { return std::byte{ v }; });
    }

    GptHeader GptHeaderLayout::load(lclosed_interval<uint64_t> lba_range, size_t block_size) const {
        GptHeader retval;

        if (!(signature == GPT_SIGNATURE)) {
            throw GptInvalidSignatureError("Bad GPT header: Invalid signature.");
        }

        {
            uint32_t revision_ = endian_load<uint32_t, std::endian::little>(revision);
            if (revision_ != GPT_REVISION) {
                throw std::runtime_error(std::format("Bad GPT header: Unexpected `revision`, expect 0x{:08x}, but got 0x{:08x}.", GPT_REVISION, revision_));
            }
        }

        {
            uint32_t header_size_ = endian_load<uint32_t, std::endian::little>(header_size);
            if (header_size_ != sizeof(GptHeaderLayout)) {
                throw std::runtime_error(std::format("Bad GPT header: Unexpected `header_size`, expect 0x{:x}, but got 0x{:x}.", sizeof(GptHeaderLayout), header_size_));
            }
        }

        if (!std::ranges::all_of(reserved_zero, [](auto v) { return v == std::byte{}; })) {
            throw std::runtime_error("Bad GPT header: `reserved_zero` field is not zero.");
        }

        {
            uint32_t partition_entry_size_ = endian_load<uint32_t, std::endian::little>(partition_entry_size);
            if (partition_entry_size_ != sizeof(GptPartitionEntryLayout)) {
                throw std::runtime_error(std::format("Bad GPT header: Unexpected `partition_entry_size`, expect 0x{:x}, but got 0x{:x}.", sizeof(GptPartitionEntryLayout), partition_entry_size_));
            }
        }

        retval.m_current_lba = current_lba.load(lba_range);
        retval.m_backup_lba = backup_lba.load(lba_range);

        if (retval.m_current_lba == retval.m_backup_lba) {
            throw std::runtime_error("Bad GPT header: `current_lba` should be different with `backup_lba`.");
        }

        retval.m_first_usable_lba = first_usable_lba.load(lba_range);
        retval.m_last_usable_lba = last_usable_lba.load(lba_range);

        if (retval.m_first_usable_lba > retval.m_last_usable_lba) {
            throw std::runtime_error("Bad GPT header: `first_usable_lba` > `last_usable_lba`.");
        }

        retval.m_guid = guid.load();

        retval.m_partition_entries_lba = partition_entries_lba.load(lba_range);

        {
            uint32_t partition_entries_num_ = endian_load<uint32_t, std::endian::little>(partition_entries_num);

            auto partition_entries_lba_range_size =
                (partition_entries_num_ * sizeof(GptPartitionEntryLayout) + block_size - 1) / block_size;

            if (std::to_underlying(retval.m_partition_entries_lba) + partition_entries_lba_range_size <= lba_range.max) {
                retval.m_partition_entries_num = partition_entries_num_;
            } else {
                throw std::runtime_error("Bad GPT header: `partition_entries_num` exceeded.");
            }
        }

        retval.m_partition_entries_checksum = endian_load<uint32_t, std::endian::little>(partition_entries_checksum);

        {
            uint32_t header_checksum_ = endian_load<uint32_t, std::endian::little>(header_checksum);

            uint32_t checksum = 0;
            checksum = crc32_iso3309(checksum, signature);
            checksum = crc32_iso3309(checksum, revision);
            checksum = crc32_iso3309(checksum, header_size);
            checksum = crc32_iso3309(checksum, decltype(header_checksum){});
            checksum = crc32_iso3309(checksum, reserved_zero);
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &current_lba, 1 }));
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &backup_lba, 1 }));
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &first_usable_lba, 1 }));
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &last_usable_lba, 1 }));
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &guid, 1 }));
            checksum = crc32_iso3309(checksum, std::as_bytes(std::span{ &partition_entries_lba, 1 }));
            checksum = crc32_iso3309(checksum, partition_entries_num);
            checksum = crc32_iso3309(checksum, partition_entry_size);
            checksum = crc32_iso3309(checksum, partition_entries_checksum);

            if (header_checksum_ != checksum) {
                throw GptChecksumValidationError(std::format("Bad GPT header: Invalid checksum, expect 0x{:08x}, but got 0x{:08x}.", checksum, header_checksum_));
            }
        }

        return retval;
    }

    GptPartitionAttributes GptPartitionAttributesLayout::load() const {
        return std::bit_cast<GptPartitionAttributes>(endian_load<uint64_t, std::endian::little>(value));
    }

    void GptPartitionAttributesLayout::store(const GptPartitionAttributes& attributes) noexcept {
        endian_store<uint64_t, std::endian::little>(value, std::bit_cast<uint64_t>(attributes));
    }

    GptPartitionEntry GptPartitionEntryLayout::load(lclosed_interval<uint64_t> lba_range) const {
        GptPartitionEntry retval;

        retval.m_type_guid = type_guid.load();
        retval.m_unique_guid = unique_guid.load();
        retval.m_first_lba = first_lba.load(lba_range);
        retval.m_last_lba = last_lba.load(lba_range);

        if (retval.m_first_lba > retval.m_last_lba) {
            throw std::runtime_error("Bad GPT partition entry: `first_lba` > `last_lba`.");
        }

        retval.m_attributes = attributes.load();

        std::ranges::transform(
            name | std::views::chunk(2), retval.m_name.begin(),
            [](auto w) {
                return endian_load<char16_t, std::endian::little>(std::span<const uint8_t, 2>{ w });
            }
        );

        return retval;
    }
}

namespace vmgs {
    lclosed_interval<uint64_t> GptHeader::partition_entries_lba_range(size_t block_size) const noexcept {
        uint64_t s = static_cast<uint64_t>(m_partition_entries_num) * sizeof(GptPartitionEntryLayout);
        uint64_t n = (s + (block_size - 1)) / block_size;
        return lclosed_interval<uint64_t>{
            .min = std::to_underlying(m_partition_entries_lba), .max = std::to_underlying(m_partition_entries_lba) + n
        };
    }

    size_t GptHeader::partition_entries_size() const noexcept {
        return m_partition_entries_num * sizeof(GptPartitionEntryLayout);
    }

    Gpt Gpt::load_from(IBlockDevice& block_device) {
        auto block_size = block_device.get_block_size();
        auto lba_range = block_device.get_lba_range();

        if (lba_range.length() < 1 + 2) {  // 1 for protective MBR, 2 for two GPT header
            throw std::runtime_error("Bad GPT: Insufficient data.");
        }

        {
            auto protective_mbr = std::make_unique<std::byte[]>(block_size);

            block_device.read_blocks(0, 1, protective_mbr.get());

            if (!(protective_mbr[block_size - 2] == std::byte{ 0x55 } && protective_mbr[block_size - 1] == std::byte{ 0xaa })) {
                throw std::runtime_error("Bad GPT: No protective MBR.");
            }
        }

        GptHeader gpt_header;
        {
            auto header_block = std::make_unique<std::byte[]>(block_size);
            auto header_layout = reinterpret_cast<GptHeaderLayout*>(header_block.get());

            block_device.read_blocks(1, 1, header_block.get());

            try {
                gpt_header = header_layout->load(lba_range, block_size);
            } catch (GptChecksumValidationError& e) {
                block_device.read_blocks(std::to_underlying(header_layout->backup_lba.load(lba_range)), 1, header_block.get());
                gpt_header = header_layout->load(lba_range, block_size);
            }
        }

        std::vector<GptPartitionEntry> gpt_partition_entries;
        {
            auto partition_entries_size = gpt_header.partition_entries_size();
            auto partition_entries_lba_range = gpt_header.partition_entries_lba_range(block_size);
            auto partition_entries_lba_range_size = partition_entries_lba_range.length() * block_size;

            if (partition_entries_lba_range.contains(0)) {
                throw std::runtime_error("Bad GPT: Protective MBR overlapped with partition entries.");
            }

            if (partition_entries_lba_range.contains(std::to_underlying(gpt_header.current_lba()))) {
                throw std::runtime_error("Bad GPT: Protective MBR overlapped with partition entries.");
            }

            if (partition_entries_lba_range.contains(std::to_underlying(gpt_header.backup_lba()))) {
                throw std::runtime_error("Bad GPT: Protective MBR overlapped with partition entries.");
            }

            auto partition_entries_blocks = std::make_unique<std::byte[]>(partition_entries_lba_range_size);

            block_device.read_blocks(partition_entries_lba_range, partition_entries_blocks.get());

            auto checksum = crc32_iso3309(0, partition_entries_blocks.get(), partition_entries_size);

            if (checksum == gpt_header.m_partition_entries_checksum) {
                gpt_partition_entries.reserve(gpt_header.partition_entries_num());

                uint32_t i = 0;
                for (auto w : std::span{ partition_entries_blocks.get(), partition_entries_lba_range_size } | std::views::chunk(sizeof(GptPartitionEntryLayout))) {
                    if (i == gpt_header.partition_entries_num()) {
                        break;
                    }

                    gpt_partition_entries.emplace_back(reinterpret_cast<GptPartitionEntryLayout*>(w.data())->load(lba_range));

                    ++i;
                }
            } else {
                throw GptChecksumValidationError(std::format("Bad GPT header: Invalid partition entries checksum, expect 0x{:08x}, but got 0x{:08x}.", checksum, gpt_header.m_partition_entries_checksum));
            }
        }

        return Gpt{ std::move(gpt_header), std::move(gpt_partition_entries) };
    }
}
