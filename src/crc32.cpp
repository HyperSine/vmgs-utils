#include "crc32.hpp"

#if defined(WIN32)
#include <windows.h>

// available on Windows XP and above, implemented in ntdll.dll
extern "C" NTSYSAPI
DWORD NTAPI RtlComputeCrc32(_In_ DWORD Initial, _In_reads_bytes_(BufferSize) LPCVOID Buffer, _In_ ULONG BufferSize);

namespace vmgs {
    uint32_t crc32_iso3309(uint32_t initial, const void* data, size_t size) noexcept {
        while (0 < size) {
            auto l = static_cast<ULONG>(std::min<size_t>(size, std::numeric_limits<ULONG>::max()));

            initial = RtlComputeCrc32(initial, data, l);

            data = reinterpret_cast<const char*>(data) + l;
            size = size - l;
        }
        return initial;
    }
}
#else
#include <zlib.h>

namespace vmgs {
    uint32_t crc32_iso3309(uint32_t initial, const void *data, size_t size) noexcept {
        return crc32_z(initial, reinterpret_cast<const Bytef*>(data), size);
    }
}
#endif
