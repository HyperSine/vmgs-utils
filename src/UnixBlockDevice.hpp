#pragma once
#include <string>
#include <utility>

#include "IBlockDevice.hpp"

namespace vmgs {
    class UnixBlockDevice : public IBlockDevice {
    private:
        int m_fd;
        uint32_t m_block_size;
        uint64_t m_device_size;

        UnixBlockDevice() noexcept
            : m_fd(-1), m_block_size{}, m_device_size{} {}

    public:
        UnixBlockDevice(UnixBlockDevice&& other) noexcept :
            m_fd{ std::exchange(other.m_fd, -1) },
            m_block_size{ std::exchange(other.m_block_size, 0) },
            m_device_size{ std::exchange(other.m_device_size, 0) } {}

        UnixBlockDevice(const UnixBlockDevice& other) = delete;

        virtual ~UnixBlockDevice() override;

        UnixBlockDevice& operator=(UnixBlockDevice&& other) noexcept(noexcept(this->close())) {
            this->close();

            m_fd = std::exchange(other.m_fd, -1);
            m_block_size = std::exchange(other.m_block_size, 0);
            m_device_size = std::exchange(other.m_device_size, 0);

            return *this;
        }

        UnixBlockDevice& operator=(const UnixBlockDevice& other) = delete;

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
        static UnixBlockDevice open(std::string_view path, bool wrtiable);
    };
}
