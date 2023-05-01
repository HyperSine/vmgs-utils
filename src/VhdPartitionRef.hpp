#pragma once
#include <cstddef>
#include <cstdint>
#include "interval.hpp"
#include "IBlockDevice.hpp"
#include "VhdDisk.hpp"
#include "Gpt.hpp"

namespace vmgs {
    class VhdPartitionRef : public IBlockDevice {
    private:
        VhdDisk& m_disk;
        lclosed_interval<uint64_t> m_lba_range;

    public:
        VhdPartitionRef(VhdDisk& disk, const GptPartitionEntry& partition) noexcept
            : m_disk{ disk }, m_lba_range{ partition.lba_range() } {}

        [[nodiscard]]
        virtual size_t get_block_size() const override {
            return m_disk.get_block_size();
        }

        [[nodiscard]]
        virtual uint64_t get_block_count() const override {
            return m_lba_range.length();
        }

        virtual void read_blocks(uint64_t lba, uint32_t n, void* buf) {
            return m_disk.read_blocks(m_lba_range.min + lba, n, buf);
        }

        virtual void write_blocks(uint64_t lba, uint32_t n, const void* buf) {
            return m_disk.write_blocks(m_lba_range.min + lba, n, buf);
        }
    };
}
