/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Emex64Platform.h>

#define DEFAULT_WIDTH   640
#define DEFAULT_HEIGHT  480

#pragma pack(1)

typedef struct {
    VENDOR_DEVICE_PATH Vendor;
    EFI_DEVICE_PATH_PROTOCOL End;
} GOP_DEVICE_PATH;

#pragma pack ()

STATIC GOP_DEVICE_PATH mDevicePath = {
    {
        { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0 } },
        { 0 }
    },
    {
        END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
};

STATIC UINT64                                   mRegs;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL             mGop;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        mMode;
STATIC EFI_GRAPHICS_OUTPUT_MODE_INFORMATION     mInfo;
STATIC FRAME_BUFFER_CONFIGURE                   *mBltConfigure;
STATIC UINTN                                    mBltConfigureSize;

STATIC UINT64 FindDisplay(VOID)
{
    UINT64 Ecam;
    UINT64 Cfg;
    UINT64 Id;
    UINT64 Size;
    UINT64 Base;
    UINTN Dev;

    if((MmioRead64(EMEX64_PCIE_RC_ID) & 0xFFFFFFFF) != EMEX64_PCIE_RC_MAGIC)
    {
        return 0;
    }

    Ecam = MmioRead64(EMEX64_PCIE_RC_ECAM);
    for(Dev = 0; Dev < 32; Dev++)
    {
        Cfg = Ecam + LShiftU64(Dev, 16);
        Id = MmioRead64(Cfg + EMEX64_PCIE_CFG_ID);
        if(((Id & 0xFFFF) != EMEX64_GPU_VENDOR_ID) || (((Id >> 16) & 0xFFFF) != EMEX64_GPU_DEVICE_ID))
        {
            continue;
        }

        Size = MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR_SIZE (0));
        if(Size == 0)
        {
            return 0;
        }

        if((MmioRead64(Cfg + EMEX64_PCIE_CFG_CONTROL) & EMEX64_PCIE_CONTROL_MEM) == 0)
        {
            MmioWrite64(Cfg + EMEX64_PCIE_CFG_CONTROL, 0);
            Base = (MmioRead64(EMEX64_PCIE_RC_MMIO_BASE) + MmioRead64(EMEX64_PCIE_RC_MMIO_SIZE) - Size) & ~(Size - 1);
            MmioWrite64(Cfg + EMEX64_PCIE_CFG_BAR(0), Base);
        }

        MmioWrite64(Cfg + EMEX64_PCIE_CFG_CONTROL, EMEX64_PCIE_CONTROL_MEM | EMEX64_PCIE_CONTROL_MASTER);
        return MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR(0));
    }

    return 0;
}

STATIC VOID PreferredMode(OUT UINT32 *Width,
                          OUT UINT32 *Height)
{
    UINT64 Bytes;

    MmioWrite64(mRegs + EMEX64_GPU_DP_AUX_ADDR, EMEX64_GPU_AUX_EDID | 54);
    Bytes = MmioRead64(mRegs + EMEX64_GPU_DP_AUX_DATA);

    *Width = (UINT32)(((Bytes >> 16) & 0xFF) | (((Bytes >> 36) & 0xF) << 8));
    *Height = (UINT32)(((Bytes >> 40) & 0xFF) | (((Bytes >> 60) & 0xF) << 8));

    if((*Width < 256) || (*Height < 256) || (*Width > 4096) || (*Height > 4096))
    {
        *Width = DEFAULT_WIDTH;
        *Height = DEFAULT_HEIGHT;
    }
}

STATIC EFI_STATUS EFIAPI GopQueryMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
                                      IN UINT32 ModeNumber,
                                      OUT UINTN *SizeOfInfo,
                                      OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info)
{
    if((SizeOfInfo == NULL) || (Info == NULL) || (ModeNumber >= mMode.MaxMode))
    {
        return EFI_INVALID_PARAMETER;
    }

    *Info = AllocateCopyPool(sizeof (mInfo), &mInfo);
    if(*Info == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    *SizeOfInfo = sizeof (mInfo);
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI GopSetMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
                                    IN UINT32 ModeNumber)
{
    if(ModeNumber >= mMode.MaxMode)
    {
        return EFI_UNSUPPORTED;
    }

    ZeroMem((VOID *)(UINTN)mMode.FrameBufferBase, mMode.FrameBufferSize);

    MmioWrite64(mRegs + EMEX64_GPU_SCAN_CTRL, 0);
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_BASE, mMode.FrameBufferBase);
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_WIDTH, mInfo.HorizontalResolution);
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_HEIGHT, mInfo.VerticalResolution);
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_STRIDE, mInfo.PixelsPerScanLine * sizeof (UINT32));
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_FORMAT, EMEX64_GPU_FORMAT_XRGB8888);
    MmioWrite64(mRegs + EMEX64_GPU_SCAN_CTRL, 1);

    if((MmioRead64(mRegs + EMEX64_GPU_SCAN_STATUS) & 3) != 1)
    {
        DEBUG((DEBUG_ERROR, "GopDxe: the scanout refused the mode (status 0x%Lx)\n", MmioRead64 (mRegs + EMEX64_GPU_SCAN_STATUS)));
        return EFI_DEVICE_ERROR;
    }

    mMode.Mode = ModeNumber;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI GopBlt(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
                                IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer  OPTIONAL,
                                IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
                                IN UINTN SourceX,
                                IN UINTN SourceY,
                                IN UINTN DestinationX,
                                IN UINTN DestinationY,
                                IN UINTN Width,
                                IN UINTN Height,
                                IN UINTN Delta OPTIONAL)
{
    EFI_STATUS Status;
    EFI_TPL OriginalTpl;

    OriginalTpl = gBS->RaiseTPL (TPL_NOTIFY);
    Status = FrameBufferBlt(mBltConfigure, BltBuffer, BltOperation, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    gBS->RestoreTPL(OriginalTpl);
    return Status;
}

EFI_STATUS EFIAPI GopDxeInitialize(IN EFI_HANDLE ImageHandle,
                                   IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_HANDLE Handle;
    UINT32 Width;
    UINT32 Height;
    UINTN Pages;
    VOID *FrameBuffer;
    UINTN Index;

    mRegs = FindDisplay();
    if(mRegs == 0)
    {
        DEBUG((DEBUG_INFO, "GopDxe: no display controller\n"));
        return EFI_UNSUPPORTED;
    }

    if((MmioRead64(mRegs + EMEX64_GPU_DP_STATUS) & EMEX64_GPU_DP_STATUS_HPD) == 0)
    {
        DEBUG((DEBUG_INFO, "GopDxe: no monitor on the DisplayPort connector\n"));
        return EFI_UNSUPPORTED;
    }

    MmioWrite64(mRegs + EMEX64_GPU_DP_LINK, 1);
    for(Index = 0; Index < 1000; Index++)
    {
        if((MmioRead64 (mRegs + EMEX64_GPU_DP_STATUS) & EMEX64_GPU_DP_STATUS_LINK) != 0)
        {
            break;
        }
        
        MicroSecondDelay(100);
    }

    if(Index == 1000)
    {
        DEBUG((DEBUG_ERROR, "GopDxe: the DisplayPort link didn't come up\n"));
        return EFI_DEVICE_ERROR;
    }

    PreferredMode(&Width, &Height);

    Pages = EFI_SIZE_TO_PAGES((UINTN)Width * Height * sizeof (UINT32));
    FrameBuffer = AllocateReservedPages(Pages);
    if(FrameBuffer == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    mInfo.Version = 0;
    mInfo.HorizontalResolution = Width;
    mInfo.VerticalResolution = Height;
    mInfo.PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
    mInfo.PixelsPerScanLine = Width;
    mMode.MaxMode = 1;
    mMode.Mode = 0;
    mMode.Info = &mInfo;
    mMode.SizeOfInfo = sizeof (mInfo);
    mMode.FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)FrameBuffer;
    mMode.FrameBufferSize = (UINTN)Width * Height * sizeof (UINT32);

    mGop.QueryMode = GopQueryMode;
    mGop.SetMode = GopSetMode;
    mGop.Blt = GopBlt;
    mGop.Mode = &mMode;

    mBltConfigureSize = 0;
    Status = FrameBufferBltConfigure(FrameBuffer, &mInfo, NULL, &mBltConfigureSize);
    if(Status == RETURN_BUFFER_TOO_SMALL)
    {
        mBltConfigure = AllocatePool(mBltConfigureSize);
        if(mBltConfigure == NULL)
        {
            return EFI_OUT_OF_RESOURCES;
        }
        Status = FrameBufferBltConfigure(FrameBuffer, &mInfo, mBltConfigure, &mBltConfigureSize);
    }

    if(EFI_ERROR (Status))
    {
        return Status;
    }

    Status = GopSetMode(&mGop, 0);
    if(EFI_ERROR(Status))
    {
        return Status;
    }

    DEBUG((DEBUG_INFO, "GopDxe: %ux%u XRGB8888 at 0x%p, registers at 0x%Lx\n", Width, Height, FrameBuffer, mRegs));

    CopyGuid(&mDevicePath.Vendor.Guid, &gEmex64DisplayDevicePathGuid);
    Handle = NULL;
    return gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDevicePathProtocolGuid, &mDevicePath, &gEfiGraphicsOutputProtocolGuid, &mGop, NULL);
}
