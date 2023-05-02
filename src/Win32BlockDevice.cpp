#include "Win32BlockDevice.hpp"
#include <cassert>
#include <system_error>

namespace vmgs {
    void Win32BlockDevice::read_blocks(uint64_t lba, uint32_t n, void* buf) {
        if (0 < n) {
            if (!SetFilePointerEx(m_handle, std::bit_cast<LARGE_INTEGER>(lba * m_block_size), NULL, FILE_BEGIN)) {
                throw std::system_error(GetLastError(), std::system_category());
            }

            std::unique_ptr<std::byte[]> aligned_buf;
            void* aligned_ptr = nullptr;
            if ((reinterpret_cast<uintptr_t>(buf) & m_alignment_mask) != 0) {   // when `buf` is not aligned
                size_t al = m_alignment_mask + 1;
                size_t sz = ALIGNED_BUFFER_SIZE_IN_BLOCKS * m_block_size;

                if (al <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                    aligned_buf = std::make_unique<std::byte[]>(sz);
                    aligned_ptr = aligned_buf.get();
                } else {
                    size_t alloc_sz = sz + al;
                    aligned_buf = std::make_unique<std::byte[]>(alloc_sz);
                    aligned_ptr = aligned_buf.get();
                    std::align(al, sz, aligned_ptr, alloc_sz);
                }
            }

            std::byte* buf_ptr = static_cast<std::byte*>(buf);
            do {
                DWORD len;
                if (aligned_ptr) {
                    len = static_cast<DWORD>(std::min<size_t>(n, ALIGNED_BUFFER_SIZE_IN_BLOCKS));
                } else {
                    len = static_cast<DWORD>(std::min<size_t>(n, std::numeric_limits<DWORD>::max() / m_block_size));
                }

                DWORD expect_size = len * m_block_size;
                if (aligned_ptr) {
                    memcpy(aligned_ptr, buf_ptr, expect_size);
                }

                DWORD actual_size;

                if (!ReadFile(m_handle, aligned_ptr ? aligned_ptr : buf_ptr, expect_size, &actual_size, NULL)) {
                    throw std::system_error(GetLastError(), std::system_category());
                }

                if (actual_size == 0) {
                    throw std::system_error(ERROR_HANDLE_EOF, std::system_category());
                } else {
                    assert(expect_size == actual_size);
                }

                n -= len;
                buf_ptr += actual_size;
            } while (0 < n);
        }
    }

    void Win32BlockDevice::write_blocks(uint64_t lba, uint32_t n, const void* buf) {
        if (0 < n) {
            if (!SetFilePointerEx(m_handle, std::bit_cast<LARGE_INTEGER>(lba * m_block_size), NULL, FILE_BEGIN)) {
                throw std::system_error(GetLastError(), std::system_category());
            }

            std::unique_ptr<std::byte[]> aligned_buf;
            void* aligned_ptr = nullptr;
            if ((reinterpret_cast<uintptr_t>(buf) & m_alignment_mask) != 0) {   // when `buf` is not aligned
                size_t al = m_alignment_mask + 1;
                size_t sz = ALIGNED_BUFFER_SIZE_IN_BLOCKS * m_block_size;

                if (al <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
                    aligned_buf = std::make_unique<std::byte[]>(sz);
                    aligned_ptr = aligned_buf.get();
                } else {
                    size_t alloc_sz = sz + al;
                    aligned_buf = std::make_unique<std::byte[]>(alloc_sz);
                    aligned_ptr = aligned_buf.get();
                    std::align(al, sz, aligned_ptr, alloc_sz);
                }
            }

            const std::byte* buf_ptr = static_cast<const std::byte*>(buf);
            do {
                DWORD len;
                if (aligned_ptr) {
                    len = static_cast<DWORD>(std::min<size_t>(n, ALIGNED_BUFFER_SIZE_IN_BLOCKS));
                } else {
                    len = static_cast<DWORD>(std::min<size_t>(n, std::numeric_limits<DWORD>::max() / m_block_size));
                }

                DWORD expect_size = len * m_block_size;
                if (aligned_ptr) {
                    memcpy(aligned_ptr, buf_ptr, expect_size);
                }

                DWORD actual_size;

                if (!WriteFile(m_handle, aligned_ptr ? aligned_ptr : buf_ptr, expect_size, &actual_size, NULL)) {
                    throw std::system_error(GetLastError(), std::system_category());
                }

                if (actual_size == 0) {
                    throw std::system_error(ERROR_HANDLE_EOF, std::system_category());
                } else {
                    assert(expect_size == actual_size);
                }

                n -= len;
                buf_ptr += actual_size;
            } while (0 < n);
        }
    }

    void Win32BlockDevice::close() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            if (!CloseHandle(m_handle)) {
                throw std::system_error(GetLastError(), std::system_category());
            }
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    Win32BlockDevice Win32BlockDevice::open(std::wstring_view path, bool writable) {
        Win32BlockDevice retval;

        retval.m_handle = CreateFileW(path.data(), GENERIC_READ | (writable ? GENERIC_WRITE : 0), FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (retval.m_handle == INVALID_HANDLE_VALUE) {
            throw std::system_error(GetLastError(), std::system_category());
        }

        DISK_GEOMETRY geometry;
        GET_LENGTH_INFORMATION length_information;
        FILE_ALIGNMENT_INFO file_alignment_info;
        {
            DWORD return_length;

            if (!DeviceIoControl(retval.m_handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geometry, sizeof(geometry), &return_length, NULL)) {
                throw std::system_error(GetLastError(), std::system_category());
            }

            if (!DeviceIoControl(retval.m_handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &length_information, sizeof(length_information), &return_length, NULL)) {
                throw std::system_error(GetLastError(), std::system_category());
            }

            if (!GetFileInformationByHandleEx(retval.m_handle, FileAlignmentInfo, &file_alignment_info, sizeof(file_alignment_info))) {
                throw std::system_error(GetLastError(), std::system_category());
            }
        }

        return retval;
    }
}
