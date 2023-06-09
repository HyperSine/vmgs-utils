cmake_minimum_required(VERSION 3.18)
project(vmgs-utils)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 REQUIRED)

if(WIN32)
    pybind11_add_module(
        _vmgs
            src/concepts.hpp
            src/endian_storage.hpp
            src/interval.hpp
            src/crc32.hpp
            src/crc32.cpp
            src/IBlockDevice.hpp
            src/IBlockDevice.cpp
            src/Win32BlockDevice.hpp
            src/Win32BlockDevice.cpp
            src/Gpt.hpp
            src/Gpt.cpp
            src/VhdDisk.hpp
            src/VhdDisk.cpp
            src/VhdPartitionRef.hpp
            src/VhdPartitionRef.cpp
            src/Vmgs.hpp
            src/Vmgs.cpp
            src/py.hpp
            src/init.hpp
            src/init.cpp
    )
    target_compile_definitions(_vmgs PRIVATE NOMINMAX _NTSCSI_USER_MODE_)
    target_link_libraries(_vmgs PRIVATE ntdll virtdisk)
else ()
    find_package(ZLIB REQUIRED)
    pybind11_add_module(
        _vmgs
            src/concepts.hpp
            src/endian_storage.hpp
            src/interval.hpp
            src/crc32.hpp
            src/crc32.cpp
            src/IBlockDevice.hpp
            src/IBlockDevice.cpp
            src/UnixBlockDevice.hpp
            src/UnixBlockDevice.cpp
            src/Vmgs.hpp
            src/Vmgs.cpp
            src/py.hpp
            src/init.hpp
            src/init.cpp
    )
    target_link_libraries(_vmgs PRIVATE ZLIB::ZLIB)
endif()

install(TARGETS _vmgs LIBRARY DESTINATION "./vmgs")
