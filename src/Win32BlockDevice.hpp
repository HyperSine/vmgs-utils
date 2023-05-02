#pragma once
#include <memory>
#include <string>
#include <utility>

#include <windows.h>

#include "IBlockDevice.hpp"

namespace vmgs {
    class Win32BlockDevice : public IBlockDevice {
    public:
        static constexpr size_t ALIGNED_BUFFER_SIZE_IN_BLOCKS = 8;

    private:
        HANDLE m_handle;
        size_t m_block_size;
        uint64_t m_device_size;
        uintptr_t m_alignment_mask;

        Win32BlockDevice() noexcept
            : m_handle{ INVALID_HANDLE_VALUE }, m_block_size{}, m_device_size{}, m_alignment_mask{} {}

    public:
        Win32BlockDevice(Win32BlockDevice&& other) noexcept :
            m_handle{ std::exchange(other.m_handle, INVALID_HANDLE_VALUE) },
            m_block_size{ std::exchange(other.m_block_size, 0) },
            m_device_size{ std::exchange(other.m_device_size, 0) },
            m_alignment_mask{ std::exchange(other.m_alignment_mask, 0) } {}

        Win32BlockDevice(const Win32BlockDevice& other) = delete;

        virtual ~Win32BlockDevice() noexcept override {
            this->close();
        }

        Win32BlockDevice& operator=(Win32BlockDevice&& other) noexcept(noexcept(this->close())) {
            this->close();

            m_handle = std::exchange(other.m_handle, INVALID_HANDLE_VALUE);
            m_block_size = std::exchange(other.m_block_size, 0);
            m_device_size = std::exchange(other.m_device_size, 0);
            m_alignment_mask = std::exchange(other.m_alignment_mask, 0);

            return *this;
        }

        Win32BlockDevice& operator=(const Win32BlockDevice& other) = delete;

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

        void close();

        [[nodiscard]]
        static Win32BlockDevice open(std::wstring_view path, bool writable = false);
    };
}
