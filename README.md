# vmgs-utils

A python package that helps you edit Hyper-V's VMGS file so that you can, for example, modify your Hyper-V vm's nvram, replace your vm's UEFI platform key, or something else.

## 1. How it works

See here. // todo

## 2. Build and Install

```console
$ git clone https://github.com/HyperSine/vmgs-utils.git
$ cd vmgs-utils

// make sure you have python `build` package

$ python -m build --wheel .
$ pip install --no-index -f dist vmgs-utils
```

## 3. Example

```py
import io, struct, uuid
import vmgs

# your VM's VMGS file here
filepath = r'D:\Windows-10-latest-x64\Virtual Machines\4EFD7155-DBDB-4913-99AD-51F9CC33072C.vmgs'

with vmgs.VmgsIO(file = filepath) as vmgs_f:
    vmgs_json = vmgs.vmgs_decode(vmgs_f.read())

    # print vmgs content
    print(vmgs_json)

    # Now let's modify your VM's UEFI platform key(PK)
    
    # build a EFI_SIGNATURE_LIST struct
    with io.BytesIO() as bio:
        with open('<your new UEFI PK cert file>', 'rb') as cert_f:
            cert_data = cert_f.read()
    
        # EFI_SIGNATURE_LIST.SignatureType = EFI_CERT_X509_GUID
        bio.write(uuid.UUID('A5C059A1-94E4-4AA7-87B5-AB155C2BF072').bytes_le)
        
        # EFI_SIGNATURE_LIST.SignatureListSize = (16 + 4 + 4 + 4 + 16 + <DER cert file size>)
        # 16 for EFI_SIGNATURE_LIST.SignatureType
        # 4 for EFI_SIGNATURE_LIST.SignatureListSize
        # 4 for EFI_SIGNATURE_LIST.SignatureHeaderSize
        # 4 for EFI_SIGNATURE_LIST.SignatureSize
        # 16 for EFI_SIGNATURE_DATA.SignatureOwner
        bio.write(struct.pack('<I', 16 + 4 + 4 + 4 + 16 + len(cert_data)))
    
        # EFI_SIGNATURE_LIST.SignatureHeaderSize = 0
        bio.write(struct.pack('<I', 0))
    
        # EFI_SIGNATURE_LIST.SignatureSize = (16 + <DER cert file size>)
        # 16 for EFI_SIGNATURE_DATA.SignatureOwner        
        bio.write(struct.pack('<I', 16 + len(cert_data)))
    
        # EFI_SIGNATURE_DATA.SignatureOwner = <your own GUID>
        bio.write(uuid.UUID('a33390a2-b69f-4e53-8379-f03f86d53564').bytes_le)
        
        # EFI_SIGNATURE_LIST.SignatureData
        bio.write(cert_data)
    
        new_pk = bio.getvalue()
    
    # `ac6b8dc1-3257-4a70-b1b2-a9c9215659ad` is the instance ID of Microsoft Virtual BIOS.
    # You can find it under `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Virtualization\VirtualDevices`.
    #
    # `8be4df61-93ca-11d2-aa0d-00e098032b8c` is EFI_GLOBAL_VARIABLE.
    vmgs_json \
        ["Devices"]["ac6b8dc1-3257-4a70-b1b2-a9c9215659ad"]["States"] \
        ["Nvram"]["Vendors"]["8be4df61-93ca-11d2-aa0d-00e098032b8c"]["Variables"] \
        ["PK"]["Data"] = list(new_pk)

    # The VM's nvram wll be updated after the following write.
    # You can start the VM and check if the VM's UEFI PK has been changed.
    vmgs_f.write(vmgs.vmgs_encode(vmgs_json))
```

## 3. Demo

The following is a video where I replaced my VM's UEFI platform key from `Microsoft Hyper-V Firmware PK` to my own PK `Localhost UEFI Platform Key Certificate`:

![demo.gif](demo.gif)
