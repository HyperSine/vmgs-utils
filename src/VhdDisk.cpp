#include "VhdDisk.hpp"
#include <cassert>
#include <system_error>

#include <initguid.h>
#include <virtdisk.h>
#include <scsi.h>

#include "endian_storage.hpp"

namespace vmgs {
    void VhdDisk::read_blocks(uint64_t lba, uint32_t n, void* buf) {
        if (0 < n) {
            DWORD win32_err;

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

            win32_err = RawSCSIVirtualDisk(m_handle, &scsi_req, RAW_SCSI_VIRTUAL_DISK_FLAG_NONE, &scsi_rsp);
            if (win32_err != ERROR_SUCCESS) {
                throw std::system_error(win32_err, std::system_category());
            }

            if (scsi_rsp.Version1.ScsiStatus == SCSISTAT_GOOD) {
                assert(scsi_rsp.Version1.DataTransferLength == scsi_req.Version1.DataTransferLength);
            } else {
                assert(scsi_rsp.Version1.DataTransferLength == 0);
                throw std::system_error(ERROR_DEVICE_HARDWARE_ERROR, std::system_category());
            }
        }
    }

    void VhdDisk::write_blocks(uint64_t lba, uint32_t n, const void* buf) {
        if (0 < n) {
            DWORD win32_err;

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

            win32_err = RawSCSIVirtualDisk(m_handle, &scsi_req, RAW_SCSI_VIRTUAL_DISK_FLAG_NONE, &scsi_rsp);
            if (win32_err != ERROR_SUCCESS) {
                throw std::system_error(win32_err, std::system_category());
            }

            if (scsi_rsp.Version1.ScsiStatus == SCSISTAT_GOOD) {
                assert(scsi_rsp.Version1.DataTransferLength == scsi_req.Version1.DataTransferLength);
            } else {
                assert(scsi_rsp.Version1.DataTransferLength == 0);
                throw std::system_error(ERROR_DEVICE_HARDWARE_ERROR, std::system_category());
            }
        }
    }

    void VhdDisk::attach() {
        // ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST is needed for RawSCSIVirtualDisk
        auto win32_err = AttachVirtualDisk(m_handle, NULL, ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST, 0, NULL, NULL);
        if (win32_err != ERROR_SUCCESS) {
            throw std::system_error(win32_err, std::system_category());
        }
    }

    void VhdDisk::detach() {
        auto win32_err = DetachVirtualDisk(m_handle, DETACH_VIRTUAL_DISK_FLAG_NONE, 0);
        if (win32_err != ERROR_SUCCESS) {
            throw std::system_error(win32_err, std::system_category());
        }
    }

    void VhdDisk::close() {
        if (m_handle != NULL) {
            if (!CloseHandle(m_handle)) {
                throw std::system_error(GetLastError(), std::system_category());
            }
            m_handle = NULL;
        }
    }

    VhdDisk VhdDisk::open(std::wstring_view filepath) {
        VhdDisk retval;

        DWORD win32_err;

        VIRTUAL_STORAGE_TYPE storage_type
            { .DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD, .VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT };

        win32_err = OpenVirtualDisk(&storage_type, filepath.data(), VIRTUAL_DISK_ACCESS_ALL, OPEN_VIRTUAL_DISK_FLAG_NONE, NULL, &retval.m_handle);
        if (win32_err != ERROR_SUCCESS) {
            throw std::system_error(win32_err, std::system_category());
        }

        GET_VIRTUAL_DISK_INFO vdisk_info{ .Version = GET_VIRTUAL_DISK_INFO_SIZE };
        ULONG vdisk_info_size = sizeof(GET_VIRTUAL_DISK_INFO);

        win32_err = GetVirtualDiskInformation(retval.m_handle, &vdisk_info_size, &vdisk_info, NULL);
        if (win32_err != ERROR_SUCCESS) {
            throw std::system_error(win32_err, std::system_category());
        }

        assert(vdisk_info.Size.SectorSize == SECTOR_SIZE);

        retval.m_virtual_size = vdisk_info.Size.VirtualSize;
        retval.m_physical_size = vdisk_info.Size.PhysicalSize;

        return retval;
    }
}
