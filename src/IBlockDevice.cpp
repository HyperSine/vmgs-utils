#include "IBlockDevice.hpp"
#include <limits>

namespace vmgs {
    void IBlockDevice::read_blocks(lclosed_interval<uint64_t> lba_range, void* buf) {
        auto block_size = get_block_size();
        for (auto current_lba = lba_range.min; current_lba < lba_range.max;) {
            auto n = std::min<uint64_t>(lba_range.max - current_lba, std::numeric_limits<uint32_t>::max() / block_size);

            read_blocks(current_lba, static_cast<uint32_t>(n), buf);

            current_lba = current_lba + n;
            buf = reinterpret_cast<char*>(buf) + n * block_size;
        }
    }

    void IBlockDevice::write_blocks(lclosed_interval<uint64_t> lba_range, const void* buf) {
        auto block_size = get_block_size();
        for (auto current_lba = lba_range.min; current_lba < lba_range.max;) {
            auto n = std::min<uint64_t>(lba_range.max - current_lba, std::numeric_limits<uint32_t>::max() / block_size);

            write_blocks(current_lba, static_cast<uint32_t>(n), buf);

            current_lba = current_lba + n;
            buf = reinterpret_cast<const char*>(buf) + n * block_size;
        }
    }
}
