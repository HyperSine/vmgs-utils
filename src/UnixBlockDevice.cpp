#include "UnixBlockDevice.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include <system_error>

namespace vmgs {
    UnixBlockDevice::~UnixBlockDevice() {
        close();
    }

    void UnixBlockDevice::read_blocks(uint64_t lba, uint32_t n, void* buf) {
        if (0 < n) {
            size_t expected_size = n * m_block_size;

            if (::lseek64(m_fd, lba * m_block_size, SEEK_SET) < 0) {
                throw std::system_error(errno, std::generic_category());
            }

            auto actual_size = ::read(m_fd, buf, expected_size);
            if (actual_size < 0) {
                throw std::system_error(errno, std::generic_category());
            } else if (actual_size == 0) {
                throw std::runtime_error("Read end of device.");
            } else if (actual_size != expected_size) {
                throw std::runtime_error("Some blocks are not read.");
            }
        }
    }

    void UnixBlockDevice::write_blocks(uint64_t lba, uint32_t n, const void* buf) {
        if (0 < n) {
            size_t expected_size = n * m_block_size;

            if (::lseek64(m_fd, lba * m_block_size, SEEK_SET) < 0) {
                throw std::system_error(errno, std::generic_category());
            }

            auto actual_size = ::write(m_fd, buf, expected_size);
            if (actual_size < 0) {
                throw std::system_error(errno, std::generic_category());
            } else if (actual_size != expected_size) {
                throw std::runtime_error("Some blocks are not written.");
            }
        }
    }

    void UnixBlockDevice::close() {
        if (m_fd >= 0) {
            if (::close(m_fd) < 0) {
                throw std::system_error(errno, std::generic_category());
            }
            m_fd = -1;
        }
    }

    UnixBlockDevice UnixBlockDevice::open(std::string_view path, bool wrtiable) {
        UnixBlockDevice retval;

        retval.m_fd = ::open(path.data(), wrtiable ? O_RDWR : O_RDONLY);
        if (retval.m_fd < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        if (::ioctl(retval.m_fd, BLKSSZGET, &retval.m_block_size) < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        if (::ioctl(retval.m_fd, BLKGETSIZE64, &retval.m_device_size) < 0) {
            throw std::system_error(errno, std::generic_category());
        }

        return retval;
    }
}
