#include "Vmgs.hpp"

#include <array>
#include <ranges>
#include <memory>
#include <string>
#include <format>
#include <stdexcept>

#include "endian_storage.hpp"
#include "crc32.hpp"

namespace vmgs {
    constexpr uint32_t VMGS_DATA_HEADER_VERSION = 0x00010000;

    constexpr std::array<std::byte, 8> VMGS_DATA_HEADER_SIGNATURE =
        { std::byte{'G'}, std::byte{'U'}, std::byte{'E'}, std::byte{'S'},
          std::byte{'T'}, std::byte{'R'}, std::byte{'T'}, std::byte{'S'} };

    struct VmgsDataLocatorLayout {
        std::array<std::byte, 8> allocation_lba;
        std::array<std::byte, 8> allocation_num;
        std::array<std::byte, 4> data_size;
        std::array<std::byte, 12> reserved_zero;

        [[nodiscard]]
        VmgsDataLocator load(lclosed_interval<uint64_t> lba_range, size_t block_size) const;

        void store(const VmgsDataLocator& locator) noexcept;
    };

    static_assert(sizeof(VmgsDataLocatorLayout) == 0x20);
    static_assert(alignof(VmgsDataLocatorLayout) == alignof(std::byte));

    struct VmgsDataHeaderLayout {
        std::array<std::byte, 8> signature;
        std::array<std::byte, 4> version;
        std::array<std::byte, 4> checksum;
        std::array<std::byte, 4> sequence_number;
        std::array<std::byte, 4> header_size;
        std::array<std::byte, 4> locator_size;
        std::array<std::byte, 4> active_index;
        VmgsDataLocatorLayout locators[2];

        [[nodiscard]]
        VmgsDataHeader load(lclosed_interval<uint64_t> lba_range, size_t block_size) const;

        void store(const VmgsDataHeader& header) noexcept;
    };

    static_assert(sizeof(VmgsDataHeaderLayout) == 0x60);
    static_assert(alignof(VmgsDataHeaderLayout) == alignof(std::byte));

    [[nodiscard]]
    VmgsDataLocator VmgsDataLocatorLayout::load(lclosed_interval<uint64_t> lba_range, size_t block_size) const {
        VmgsDataLocator retval;

        retval.m_allocation_lba = endian_load<uint64_t, std::endian::little>(allocation_lba);
        retval.m_allocation_num = endian_load<uint64_t, std::endian::little>(allocation_num);
        retval.m_data_size = endian_load<uint32_t, std::endian::little>(data_size);

        if (lba_range.min <= retval.m_allocation_lba && retval.m_allocation_lba + retval.m_allocation_num <= lba_range.max) {
            // pass
        } else {
            throw std::out_of_range("Bad VMGS data locator: `allocation_lba` or `allocation_num` is/are out of range.");
        }

        if (retval.m_data_size <= retval.m_allocation_num * block_size) {
            // pass
        } else {
            throw std::out_of_range("Bad VMGS data locator: `data_size` exceeded the allocation.");
        }

        return retval;
    }

    void VmgsDataLocatorLayout::store(const VmgsDataLocator& locator) noexcept {
        endian_store<uint64_t, std::endian::little>(allocation_lba, locator.m_allocation_lba);
        endian_store<uint64_t, std::endian::little>(allocation_num, locator.m_allocation_num);
        endian_store<uint32_t, std::endian::little>(data_size, locator.m_data_size);
        std::ranges::fill(reserved_zero, std::byte{});
    }

    VmgsDataHeader VmgsDataHeaderLayout::load(lclosed_interval<uint64_t> lba_range, size_t block_size) const {
        VmgsDataHeader retval;

        if (signature != VMGS_DATA_HEADER_SIGNATURE) {
            throw std::runtime_error("Bad VMGS data header: Invalid signature.");
        }

        auto version_ = endian_load<uint32_t, std::endian::little>(version);
        if (version_ != VMGS_DATA_HEADER_VERSION) {
            throw std::runtime_error(std::format("Bad VMGS data header: Unexpected header version, expect 0x{:08x}, but got 0x{:08x}.", VMGS_DATA_HEADER_VERSION, version_));
        }

        // endian_load<uint32_t, std::endian::little>(checksum);

        retval.m_sequence_number = endian_load<uint32_t, std::endian::little>(sequence_number);

        auto header_size_ = endian_load<uint32_t, std::endian::little>(header_size);
        if (header_size_ != sizeof(VmgsDataHeaderLayout)) {
            throw std::runtime_error(std::format("Bad VMGS data header: Unexpected header size, expect 0x{:x}, but got 0x{:x}.", sizeof(VmgsDataHeaderLayout), header_size_));
        }

        auto locator_size_ = endian_load<uint32_t, std::endian::little>(locator_size);
        if (locator_size_ != sizeof(VmgsDataLocatorLayout)) {
            throw std::runtime_error(std::format("Bad VMGS data header: Unexpected locator size, expect 0x{:x}, but got 0x{:x}.", sizeof(VmgsDataLocatorLayout), locator_size_));
        }

        retval.m_active_index = endian_load<uint32_t, std::endian::little>(active_index);
        if (retval.m_active_index >= std::size(retval.m_locators)) {
            throw std::runtime_error(std::format("Bad VMGS data header: Unexpected active index, expect to be less than {:d}, but got {:d}.", std::size(retval.m_locators), retval.m_active_index));
        }

        retval.m_locators[0] = locators[0].load(lba_range, block_size);
        retval.m_locators[1] = locators[1].load(lba_range, block_size);

        {
            uint32_t checksum_ = endian_load<uint32_t, std::endian::little>(checksum);

            uint32_t expect_checksum = 0;
            expect_checksum = crc32_iso3309(expect_checksum, signature);
            expect_checksum = crc32_iso3309(expect_checksum, version);
            expect_checksum = crc32_iso3309(expect_checksum, decltype(checksum){});
            expect_checksum = crc32_iso3309(expect_checksum, sequence_number);
            expect_checksum = crc32_iso3309(expect_checksum, header_size);
            expect_checksum = crc32_iso3309(expect_checksum, locator_size);
            expect_checksum = crc32_iso3309(expect_checksum, active_index);
            expect_checksum = crc32_iso3309(expect_checksum, std::span{locators });

            if (expect_checksum != checksum_) {
                throw std::runtime_error(std::format("Bad VMGS data header: Invalid checksum, expect 0x{:08x}, but got 0x{:08x}.", expect_checksum, checksum_));
            }
        }

        return retval;
    }

    void VmgsDataHeaderLayout::store(const VmgsDataHeader& header) noexcept {
        signature = VMGS_DATA_HEADER_SIGNATURE;
        endian_store<uint32_t, std::endian::little>(version, VMGS_DATA_HEADER_VERSION);
        std::ranges::fill(checksum, std::byte{});
        endian_store<uint32_t, std::endian::little>(sequence_number, header.m_sequence_number);
        endian_store<uint32_t, std::endian::little>(header_size, sizeof(VmgsDataHeaderLayout));
        endian_store<uint32_t, std::endian::little>(locator_size, sizeof(VmgsDataLocatorLayout));
        endian_store<uint32_t, std::endian::little>(active_index, header.m_active_index);

        locators[0].store(header.m_locators[0]);
        locators[1].store(header.m_locators[1]);

        endian_store<uint32_t, std::endian::little>(checksum, crc32_iso3309(0, this, sizeof(VmgsDataHeaderLayout)));
    }
}

namespace vmgs {
    void VmgsDataLocator::update_data_size(uint32_t new_size, size_t block_size) {
        if (new_size <= m_allocation_num * block_size) {
            m_data_size = new_size;
        } else {
            throw std::runtime_error("New data size exceeds allocation range.");
        }
    }

    void VmgsData::store_to(IBlockDevice& partition_dev) const {
        auto block_size = partition_dev.get_block_size();
        auto lba_range = partition_dev.get_lba_range();

        auto header0_block = std::make_unique<std::byte[]>(block_size);
        auto header1_block = std::make_unique<std::byte[]>(block_size);

        partition_dev.read_blocks(0, 1, header0_block.get());
        partition_dev.read_blocks(1, 1, header1_block.get());

        auto header0_layout = reinterpret_cast<VmgsDataHeaderLayout*>(header0_block.get());
        auto header1_layout = reinterpret_cast<VmgsDataHeaderLayout*>(header1_block.get());

        header0_layout->store(m_headers[0]);
        header1_layout->store(m_headers[1]);

        partition_dev.write_blocks(0, 1, header0_block.get());
        partition_dev.write_blocks(1, 1, header1_block.get());
    }

    VmgsData VmgsData::load_from(IBlockDevice& partition_dev) {
        VmgsData retval;

        auto block_size = partition_dev.get_block_size();
        auto lba_range = partition_dev.get_lba_range();

        auto header0_block = std::make_unique<std::byte[]>(block_size);
        auto header1_block = std::make_unique<std::byte[]>(block_size);

        partition_dev.read_blocks(0, 1, header0_block.get());
        partition_dev.read_blocks(1, 1, header1_block.get());

        auto header0_layout = reinterpret_cast<VmgsDataHeaderLayout*>(header0_block.get());
        auto header1_layout = reinterpret_cast<VmgsDataHeaderLayout*>(header1_block.get());

        retval.m_headers[0] = header0_layout->load(lba_range, block_size);
        retval.m_headers[1] = header1_layout->load(lba_range, block_size);

        if (retval.m_headers[0].m_sequence_number == retval.m_headers[1].m_sequence_number + 1) {
            // pass
        } else if (retval.m_headers[0].m_sequence_number + 1 == retval.m_headers[1].m_sequence_number) {
            // pass
        } else {
            throw std::runtime_error("Bad VMGS: There is a gap between two VMGS headers' sequence number.");
        }

        return retval;
    }
}

#include "init.hpp"

#if defined(WIN32)
#include "Win32BlockDevice.hpp"
#include "VhdDisk.hpp"
#include "VhdPartitionRef.hpp"
#include "Gpt.hpp"
#else
#include "UnixBlockDevice.hpp"
#endif

namespace vmgs {
    class VmgsIO {
        private:
            std::unique_ptr<IBlockDevice> m_disk_dev;
            std::unique_ptr<IBlockDevice> m_partition_dev;
            std::unique_ptr<VmgsData> m_vmgs_data;

            VmgsIO(std::unique_ptr<IBlockDevice>&& partition_dev, std::unique_ptr<VmgsData>&& vmgs_data) noexcept
                : m_disk_dev{}, m_partition_dev{ std::move(partition_dev) }, m_vmgs_data{ std::move(vmgs_data) } {}

            VmgsIO(std::unique_ptr<IBlockDevice>&& disk_dev, std::unique_ptr<IBlockDevice>&& partition_dev, std::unique_ptr<VmgsData>&& vmgs_data) noexcept
                : m_disk_dev{ std::move(disk_dev) }, m_partition_dev{ std::move(partition_dev) }, m_vmgs_data{ std::move(vmgs_data) } {}

        public:
            [[nodiscard]]
            py::bytes read() {
                auto block_size = m_partition_dev->get_block_size();
                const auto& active_locator = m_vmgs_data->active_header().active_locator();

                size_t buf_n = (active_locator.data_size() + (block_size - 1)) / block_size;
                size_t buf_size = buf_n * block_size;

                std::vector<std::byte> buf(buf_size, std::byte{});
                m_partition_dev->read_blocks(active_locator.allocation_lba(), buf_n, buf.data());

                return py::bytes{ reinterpret_cast<char*>(buf.data()), active_locator.data_size() };
            }

            void write(py::buffer buf) {
                if (py::isinstance<py::bytes>(buf) || py::isinstance<py::bytearray>(buf) || py::isinstance<py::memoryview>(buf)) {
                    auto buf_info = buf.request();
                    assert(buf_info.itemsize == 1);

                    auto block_size = m_partition_dev->get_block_size();
                    auto& active_locator = m_vmgs_data->active_header().active_locator();

                    if (buf_info.size <= std::numeric_limits<uint32_t>::max()) {
                        uint32_t n = buf_info.size / block_size;

                        size_t direct_write_size = n * block_size;
                        size_t indirect_write_size = buf_info.size - n * block_size;

                        m_partition_dev->write_blocks(active_locator.allocation_lba(), n, buf_info.ptr);

                        std::vector<std::byte> single_block(block_size, std::byte{});

                        m_partition_dev->read_blocks(active_locator.allocation_lba() + n, 1, single_block.data());
                        memcpy(single_block.data(), reinterpret_cast<std::byte*>(buf_info.ptr) + direct_write_size, indirect_write_size);
                        m_partition_dev->write_blocks(active_locator.allocation_lba() + n, 1, single_block.data());

                        m_vmgs_data->store_to(*m_partition_dev);
                    } else {
                        throw py::value_error("`buf` is too long.");
                    }
                } else {
                    throw py::type_error("`buf` argument is not a instance of bytes/bytearray/memoryview type.");
                }
            }

            void close() {
                m_partition_dev.reset();
                m_disk_dev.reset();
            }

#if defined(WIN32)
            [[nodiscard]]
            static VmgsIO from_disk(py::str path) {
                constexpr GptGuid VMGS_PARTITION_TYPE_GUID =
                    { 0x700f0c12, 0x1515, 0x4e4d, { 0x8d, 0x32, 0x53, 0xf6, 0x85, 0xbf, 0x44, 0xaf } };

                auto disk_dev = std::make_unique<VhdDisk>(VhdDisk::open(path.cast<std::wstring>()));
                auto disk_gpt = Gpt::load_from(*disk_dev);

                for (const auto& partition : disk_gpt.partitions()) {
                    if (partition.type_guid() == VMGS_PARTITION_TYPE_GUID) {
                        auto partition_dev = std::make_unique<VhdPartitionRef>(*disk_dev, partition);
                        auto vmgs_data = std::make_unique<VmgsData>(VmgsData::load_from(*partition_dev));
                        return VmgsIO{ std::move(disk_dev), std::move(partition_dev), std::move(vmgs_data) };
                    }
                }

                throw std::runtime_error("Bad VMGS: VMGS partition is not found.");
            }
#endif

            [[nodiscard]]
            static VmgsIO from_partition(py::str path, bool writable) {
#if defined(WIN32)
                auto partition_dev =
                    std::make_unique<Win32BlockDevice>(Win32BlockDevice::open(path.cast<std::wstring>(), writable));
#else
                auto partition_dev =
                    std::make_unique<UnixBlockDevice>(UnixBlockDevice::open(path.cast<std::string>(), writable));
#endif
                auto vmgs_data = std::make_unique<VmgsData>(VmgsData::load_from(*partition_dev));
                return VmgsIO{ std::move(partition_dev), std::move(vmgs_data) };
            }
    };

    template<>
    struct class_pybinder_t<VmgsIO> : pybinder_t {
        using binding_t = py::class_<VmgsIO>;

        static constexpr std::string_view binder_identifier = "vmgs.VmgsIO";

        class_pybinder_t() {
            auto [_, inserted] = pybinder_t::registered_binders().emplace(binder_identifier, this);
            assert(inserted);
        }

        virtual void declare(py::module_& m) override {
            binding_t{ m, "VmgsIO" };
        }

        virtual void make_binding(py::module_& m) override {
            m.attr("VmgsIO").cast<binding_t>()
                .def(py::init(
                    [](py::kwargs kwargs) -> VmgsIO {
                        std::optional<py::str> dev;
                        std::optional<py::str> file;
                        std::optional<py::bool_> writable;

                        if (kwargs.contains("dev")) {
                            auto obj = py::getattr(kwargs, "get")("dev");
                            if (py::isinstance<py::str>(obj)) {
                                dev = py::reinterpret_borrow<py::str>(obj);
                            } else {
                                throw py::type_error("`dev` argument is not a instance of str type.");
                            }
                        }

                        if (kwargs.contains("file")) {
                            auto obj = py::getattr(kwargs, "get")("file");
                            if (py::isinstance<py::str>(obj)) {
                                file = py::reinterpret_borrow<py::str>(obj);
                            } else {
                                throw py::type_error("`file` argument is not a instance of str type.");
                            }
                        }

                        if (kwargs.contains("writable")) {
                            auto obj = py::getattr(kwargs, "get")("writable");
                            if (py::isinstance<py::bool_>(obj)) {
                                writable = py::reinterpret_borrow<py::bool_>(obj);
                            } else {
                                throw py::type_error("`writable` argument is not a instance of bool type.");
                            }
                        }

                        if (!dev.has_value() && !file.has_value()) {
                            throw py::value_error("Missing `dev` or `file` argument.");
                        } else if (dev.has_value() && !file.has_value()) {
                            return VmgsIO::from_partition(dev.value(), writable.has_value() ? static_cast<bool>(writable.value()) : false);
                        } else if (!dev.has_value() && file.has_value()) {
#if defined(WIN32)
                            return VmgsIO::from_disk(file.value());
#else
                            throw py::not_implemented_error("`file` arguemnt is not supported on non-windows platform.");
#endif
                        } else {
                            throw py::value_error("`dev` and `file` argument conflicts.");
                        }
                    }
                ))
                .def("read", &VmgsIO::read)
                .def("write", &VmgsIO::write)
                .def("close", &VmgsIO::close)
                .def("__enter__",
                    [](VmgsIO& self) -> VmgsIO& {
                        return self;
                    }
                )
                .def("__exit__",
                    [](VmgsIO& self, py::object exc_type, py::object exc_value, py::object traceback) -> bool {
                        self.close();
                        return false;
                    }
                );
        }
    };

    namespace { class_pybinder_t<VmgsIO> _; }
}
