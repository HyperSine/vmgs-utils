#pragma once
#include <memory>
#include <string>
#include <utility>

#include <windows.h>
#include <wil/resource.h>

#include "IBlockDevice.hpp"

namespace vmgs {
    class Win32BlockDevice : public IBlockDevice {
    public:
        static constexpr size_t ALIGNED_BUFFER_SIZE_IN_BLOCKS = 8;

    private:
        wil::unique_hfile m_handle;
        size_t m_block_size;
        uint64_t m_device_size;
        uintptr_t m_alignment_mask;

        Win32BlockDevice(wil::unique_hfile&& handle, size_t block_size, uint64_t device_size, uintptr_t alignment_mask) noexcept :
            m_handle{ std::move(handle) },
            m_block_size{ block_size },
            m_device_size{ device_size },
            m_alignment_mask{ alignment_mask } {}

    public:
        [[nodiscard]]
        virtual size_t get_block_size() const override {
            return m_block_size;
        }

        [[nodiscard]]
        virtual uint64_t get_block_count() const override {
            return m_device_size / m_block_size;
        }

        virtual void read_blocks(uint64_t lba, uint32_t n, void* buf) override;

        virtual void write_blocks(uint64_t lba, uint32_t n, const void* buf) override;

        void close() {
            m_handle.reset();
        }

        [[nodiscard]]
        static Win32BlockDevice open(std::wstring_view path, bool writable = false);
    };
}
