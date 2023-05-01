#include "VhdDisk.hpp"
#include "endian_storage.hpp"
#include <scsi.h>

namespace vmgs {
    void VhdDisk::read_blocks(uint64_t lba, uint32_t n, void* buf) {
        CDB cdb{};
        SENSE_DATA sense_data{};

        RAW_SCSI_VIRTUAL_DISK_PARAMETERS scsi_req{};
        RAW_SCSI_VIRTUAL_DISK_RESPONSE scsi_rsp;

        cdb.READ16.OperationCode = SCSIOP_READ16;
        endian_store<uint64_t, std::endian::big>(cdb.READ16.LogicalBlock, lba);
        endian_store<uint32_t, std::endian::big>(cdb.READ16.TransferLength, n);

        scsi_req.Version = RAW_SCSI_VIRTUAL_DISK_VERSION_1;
        scsi_req.Version1.RSVDHandle = FALSE;
        scsi_req.Version1.DataIn = 1;
        scsi_req.Version1.CdbLength = sizeof(cdb);
        scsi_req.Version1.SenseInfoLength = sizeof(sense_data);
        scsi_req.Version1.SrbFlags = 0;
        scsi_req.Version1.DataTransferLength = n * SECTOR_SIZE;
        scsi_req.Version1.DataBuffer = buf;
        scsi_req.Version1.SenseInfo = reinterpret_cast<UCHAR*>(&sense_data);
        scsi_req.Version1.Cdb = reinterpret_cast<UCHAR*>(&cdb);

        THROW_IF_WIN32_ERROR(
            RawSCSIVirtualDisk(m_handle.get(), &scsi_req, RAW_SCSI_VIRTUAL_DISK_FLAG_NONE, &scsi_rsp)
        );

        if (scsi_rsp.Version1.ScsiStatus == SCSISTAT_GOOD) {
            WI_ASSERT(scsi_rsp.Version1.DataTransferLength == scsi_req.Version1.DataTransferLength);
        } else {
            WI_ASSERT(scsi_rsp.Version1.DataTransferLength == 0);
            THROW_WIN32(ERROR_DEVICE_HARDWARE_ERROR);
        }
    }

    void VhdDisk::write_blocks(uint64_t lba, uint32_t n, const void* buf) {
        CDB cdb{};
        SENSE_DATA sense_data{};

        RAW_SCSI_VIRTUAL_DISK_PARAMETERS scsi_req{};
        RAW_SCSI_VIRTUAL_DISK_RESPONSE scsi_rsp;

        cdb.WRITE16.OperationCode = SCSIOP_WRITE16;
        endian_store<uint64_t, std::endian::big>(cdb.WRITE16.LogicalBlock, lba);
        endian_store<uint32_t, std::endian::big>(cdb.WRITE16.TransferLength, n);

        scsi_req.Version = RAW_SCSI_VIRTUAL_DISK_VERSION_1;
        scsi_req.Version1.RSVDHandle = FALSE;
        scsi_req.Version1.DataIn = 0;
        scsi_req.Version1.CdbLength = sizeof(cdb);
        scsi_req.Version1.SenseInfoLength = sizeof(sense_data);
        scsi_req.Version1.SrbFlags = 0;
        scsi_req.Version1.DataTransferLength = n * SECTOR_SIZE;
        scsi_req.Version1.DataBuffer = const_cast<void*>(buf);
        scsi_req.Version1.SenseInfo = reinterpret_cast<UCHAR*>(&sense_data);
        scsi_req.Version1.Cdb = reinterpret_cast<UCHAR*>(&cdb);

        THROW_IF_WIN32_ERROR(
            RawSCSIVirtualDisk(m_handle.get(), &scsi_req, RAW_SCSI_VIRTUAL_DISK_FLAG_NONE, &scsi_rsp)
        );

        if (scsi_rsp.Version1.ScsiStatus == SCSISTAT_GOOD) {
            WI_ASSERT(scsi_rsp.Version1.DataTransferLength == scsi_req.Version1.DataTransferLength);
        } else {
            WI_ASSERT(scsi_rsp.Version1.DataTransferLength == 0);
            THROW_WIN32(ERROR_DEVICE_HARDWARE_ERROR);
        }
    }

    void VhdDisk::attach() {
        // ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST is needed for RawSCSIVirtualDisk
        THROW_IF_WIN32_ERROR(
            AttachVirtualDisk(m_handle.get(), NULL, ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST, 0, NULL, NULL)
        );
    }

    void VhdDisk::detach() {
        THROW_IF_WIN32_ERROR(
            DetachVirtualDisk(m_handle.get(), DETACH_VIRTUAL_DISK_FLAG_NONE, 0)
        );
    }

    void VhdDisk::close() {
        m_handle.reset();
    }

    VhdDisk VhdDisk::open(std::wstring_view filepath) {
        wil::unique_handle handle;
        uint64_t virtual_size;
        uint64_t physical_size;

        {
            VIRTUAL_STORAGE_TYPE storage_type
                { .DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD, .VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT };

            THROW_IF_WIN32_ERROR(
                OpenVirtualDisk(&storage_type, filepath.data(), VIRTUAL_DISK_ACCESS_ALL, OPEN_VIRTUAL_DISK_FLAG_NONE, NULL, wil::out_param(handle))
            );
        }

        {
            GET_VIRTUAL_DISK_INFO vdisk_info{ .Version = GET_VIRTUAL_DISK_INFO_SIZE };
            ULONG vdisk_info_size = sizeof(GET_VIRTUAL_DISK_INFO);

            THROW_IF_WIN32_ERROR(
                GetVirtualDiskInformation(handle.get(), &vdisk_info_size, &vdisk_info, NULL)
            );

            virtual_size = vdisk_info.Size.VirtualSize;
            physical_size = vdisk_info.Size.PhysicalSize;

            WI_ASSERT(vdisk_info.Size.SectorSize == SECTOR_SIZE);
        }

        return VhdDisk{ std::move(handle), virtual_size, physical_size };
    }
}
