#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <windows.h>

#include "IBlockDevice.hpp"

namespace vmgs {
    class VhdDisk : public IBlockDevice {
    public:
        // according to `Virtual Hard Disk Format Spec_10_18_06.doc`, sector length is always 512 bytes.
        static constexpr size_t SECTOR_SIZE = 512;

    private:
        HANDLE m_handle;
        uint64_t m_virtual_size;
        uint64_t m_physical_size;

        VhdDisk() noexcept
            : m_handle{ NULL }, m_virtual_size{}, m_physical_size{} {}

    public:
        VhdDisk(VhdDisk&& other) noexcept :
            m_handle{ std::exchange(other.m_handle, HANDLE{ NULL }) },
            m_virtual_size{ std::exchange(other.m_virtual_size, 0) },
            m_physical_size{ std::exchange(other.m_physical_size, 0) } {}

        VhdDisk(const VhdDisk& other) = delete;

        virtual ~VhdDisk() noexcept override {
            this->close();
        }

        VhdDisk& operator=(VhdDisk&& other) noexcept(noexcept(this->close())) {
            this->close();

            m_handle = std::exchange(other.m_handle, HANDLE{ NULL });
            m_virtual_size = std::exchange(other.m_virtual_size, 0);
            m_physical_size = std::exchange(other.m_physical_size, 0);

            return *this;
        }

        VhdDisk& operator=(const VhdDisk& other) = delete;

        [[nodiscard]]
        virtual size_t get_block_size() const noexcept override {
            return SECTOR_SIZE;
        }

        [[nodiscard]]
        virtual uint64_t get_block_count() const noexcept override {
            return m_virtual_size / SECTOR_SIZE;
        }

        [[nodiscard]]
        uint64_t get_virtual_size() const noexcept {
            return m_virtual_size;
        }

        [[nodiscard]]
        uint64_t get_physical_size() const noexcept {
            return m_physical_size;
        }

        void attach();

        virtual void read_blocks(uint64_t lba, uint32_t n, void* buf) override;

        virtual void write_blocks(uint64_t lba, uint32_t n, const void* buf) override;

        void detach();

        void close();

        [[nodiscard]]
        static VhdDisk open(std::wstring_view filepath);
    };
}
