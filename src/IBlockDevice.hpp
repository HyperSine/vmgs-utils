#pragma once
#include <cstddef>
#include <cstdint>

#include "interval.hpp"

namespace vmgs {
    struct IBlockDevice {
        [[nodiscard]]
        virtual size_t get_block_size() const = 0;

        [[nodiscard]]
        virtual uint64_t get_block_count() const = 0;

        [[nodiscard]]
        lclosed_interval<uint64_t> get_lba_range() const {
            return { .min = 0, .max = get_block_count() };
        }

        virtual void read_blocks(uint64_t lba, uint32_t n, void* buf) = 0;

        virtual void write_blocks(uint64_t lba, uint32_t n, const void* buf) = 0;

        void read_blocks(lclosed_interval<uint64_t> lba_range, void* buf);

        void write_blocks(lclosed_interval<uint64_t> lba_range, const void* buf);
    };
}
