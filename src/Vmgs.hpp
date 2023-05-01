#pragma once
#include <cstddef>
#include <cstdint>

#include "interval.hpp"
#include "IBlockDevice.hpp"

namespace vmgs {
    class VmgsDataLocator {
        friend class VmgsData;
        friend class VmgsDataHeader;
        friend struct VmgsDataLocatorLayout;
    private:
        uint64_t m_allocation_lba;
        uint64_t m_allocation_num;
        uint32_t m_data_size;

    public:
        [[nodiscard]]
        uint64_t allocation_lba() const noexcept {
            return m_allocation_lba;
        }

        [[nodiscard]]
        uint64_t allocation_num() const noexcept {
            return m_allocation_num;
        }

        [[nodiscard]]
        lclosed_interval<uint64_t> allocation_lba_range() const noexcept {
            return lclosed_interval<uint64_t>{ .min = m_allocation_lba, .max = m_allocation_lba + m_allocation_num };
        }

        [[nodiscard]]
        uint32_t data_size() const noexcept {
            return m_data_size;
        }

        [[nodiscard]]
        lclosed_interval<uint64_t> data_lba_range(size_t block_size) const noexcept {
            size_t n = (m_data_size + (block_size - 1)) / block_size;
            return lclosed_interval<uint64_t>{ .min = m_allocation_lba, .max = m_allocation_lba + n };
        }

        void update_data_size(uint32_t new_size, size_t block_size);
    };

    class VmgsDataHeader {
        friend class VmgsData;
        friend struct VmgsDataHeaderLayout;
    private:
        uint32_t m_sequence_number;
        uint32_t m_active_index;
        VmgsDataLocator m_locators[2];

    public:
        [[nodiscard]]
        uint32_t sequence_number() const noexcept {
            return m_sequence_number;
        }

        [[nodiscard]]
        uint32_t active_index() const noexcept {
            return m_active_index;
        }

        [[nodiscard]]
        VmgsDataLocator& active_locator() noexcept {
            return m_locators[m_active_index];
        }

        [[nodiscard]]
        const VmgsDataLocator& active_locator() const noexcept {
            return m_locators[m_active_index];
        }
    };

    class VmgsData {
    private:
        VmgsDataHeader m_headers[2];

    public:
        [[nodiscard]]
        VmgsDataHeader& active_header() noexcept {
            return m_headers[0].m_sequence_number > m_headers[1].m_sequence_number ? m_headers[0] : m_headers[1];
        }

        [[nodiscard]]
        const VmgsDataHeader& active_header() const noexcept {
            return m_headers[0].m_sequence_number > m_headers[1].m_sequence_number ? m_headers[0] : m_headers[1];
        }

        void store_to(IBlockDevice& partition_dev) const;

        [[nodiscard]]
        static VmgsData load_from(IBlockDevice& partition_dev);
    };
}
