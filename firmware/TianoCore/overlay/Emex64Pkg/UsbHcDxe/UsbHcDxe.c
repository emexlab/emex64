/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/Usb.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Usb2HostController.h>
#include <Emex64Platform.h>
#include <Emex64Pcie.h>

#define USB_PORTS               8
#define USB_DEFAULT_TIMEOUT_MS  5000
#define USB_MAX_TD_LENGTH       SIZE_16MB

#pragma pack (1)
typedef struct {
    UINT64 Type;
    UINT64 Target;
    UINT64 Setup;
    UINT64 Buffer;
    UINT64 Length;
    UINT64 Status;
    UINT64 Actual;
    UINT64 Reserved;
} USB_TD;

typedef struct {
    VENDOR_DEVICE_PATH Controller;
    EFI_DEVICE_PATH_PROTOCOL End;
} USB_HC_DEVICE_PATH;
#pragma pack ()

#define USB_HC_SIGNATURE  SIGNATURE_32 ('E', 'U', 'H', 'C')

typedef struct {
  UINT32 Signature;
  UINT64 Regs;
  USB_TD *Tds;
  UINTN Ports;
  EFI_USB_HC_STATE State;
  EFI_USB2_HC_PROTOCOL Usb2Hc;
  USB_HC_DEVICE_PATH DevicePath;
} USB_HC;

#define USB_HC_FROM_THIS(a) CR(a, USB_HC, Usb2Hc, USB_HC_SIGNATURE)

STATIC VOID UsbHcStart(IN USB_HC *Hc)
{
    MmioWrite64(Hc->Regs + EMEX64_USB_CMD, EMEX64_USB_CMD_RESET);
    MmioWrite64(Hc->Regs + EMEX64_USB_TDLIST, (UINT64)(UINTN)Hc->Tds);
    MmioWrite64(Hc->Regs + EMEX64_USB_STS, MAX_UINT64);
    MmioWrite64(Hc->Regs + EMEX64_USB_CMD, EMEX64_USB_CMD_RUN);
    Hc->State = EfiUsbHcStateOperational;
}

STATIC EFI_STATUS
UsbHcRun(IN USB_HC *Hc,
         IN UINT64 Type,
         IN UINT8 Address,
         IN UINT8 Endpoint,
         IN UINT64 Setup,
         IN VOID *Data,
         IN UINTN Length,
         IN UINTN TimeOut,
         OUT UINTN *Actual,
         OUT UINT32 *TransferResult)
{
    USB_TD *Td;
    UINTN Waited;
    UINT64 Code;

    *Actual = 0;
    *TransferResult = EFI_USB_NOERROR;
    if(Hc->State != EfiUsbHcStateOperational)
    {
        *TransferResult = EFI_USB_ERR_SYSTEM;
        return EFI_DEVICE_ERROR;
    }

    if(Length > USB_MAX_TD_LENGTH)
    {
        *TransferResult = EFI_USB_ERR_SYSTEM;
        return EFI_INVALID_PARAMETER;
    }

    if(TimeOut == 0)
    {
        TimeOut = USB_DEFAULT_TIMEOUT_MS;
    }

    Td = &Hc->Tds[0];
    for(Waited = 0; ; Waited++)
    {
        ZeroMem(Td, sizeof (*Td));
        Td->Type = Type;
        Td->Target = Address | LShiftU64 (Endpoint & 0xF, 8);
        Td->Setup = Setup;
        Td->Buffer = (UINT64)(UINTN)Data;
        Td->Length = Length;
        MemoryFence();

        MmioWrite64(Hc->Regs + EMEX64_USB_STS, MAX_UINT64);
        MmioWrite64(Hc->Regs + EMEX64_USB_DOORBELL, BIT0);

        while((Td->Status & EMEX64_USB_TD_DONE) == 0)
        {
            if(Waited >= TimeOut)
            {
                *TransferResult = EFI_USB_ERR_TIMEOUT;
                return EFI_TIMEOUT;
            }

            MicroSecondDelay(1000);
            Waited++;
            MemoryFence();
        }

        MemoryFence();
        *Actual = (UINTN)Td->Actual;
        if((Td->Status & EMEX64_USB_TD_ERROR) == 0)
        {
            return EFI_SUCCESS;
        }

        Code = (Td->Status >> 8) & 0xFF;
        switch(Code)
        {
            case EMEX64_USB_CC_NAK:
                if(Waited >= TimeOut)
                {
                    *TransferResult = EFI_USB_ERR_NAK | EFI_USB_ERR_TIMEOUT;
                    return EFI_TIMEOUT;
                }

                MicroSecondDelay(1000);
                continue;
            case EMEX64_USB_CC_STALL:
                *TransferResult = EFI_USB_ERR_STALL;
                return EFI_DEVICE_ERROR;
            case EMEX64_USB_CC_NO_RESPONSE:
                *TransferResult = EFI_USB_ERR_TIMEOUT;
                return EFI_DEVICE_ERROR;
            case EMEX64_USB_CC_BABBLE:
                *TransferResult = EFI_USB_ERR_BABBLE;
                return EFI_DEVICE_ERROR;
            default:
                DEBUG((DEBUG_ERROR, "UsbHcDxe: device %u, endpoint %u: completion code %Lu\n", Address, Endpoint, Code));
                *TransferResult = EFI_USB_ERR_SYSTEM;
                return EFI_DEVICE_ERROR;
        }
    }
}

STATIC EFI_STATUS EFIAPI UsbHcGetCapability(IN EFI_USB2_HC_PROTOCOL *This,
                                            OUT UINT8 *MaxSpeed,
                                            OUT UINT8 *PortNumber,
                                            OUT UINT8 *Is64BitCapable)
{
    USB_HC  *Hc;

    if((MaxSpeed == NULL) || (PortNumber == NULL) || (Is64BitCapable == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    Hc = USB_HC_FROM_THIS (This);
    *MaxSpeed = EFI_USB_SPEED_HIGH;
    *PortNumber = (UINT8)Hc->Ports;
    *Is64BitCapable = 1;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI UsbHcReset(IN EFI_USB2_HC_PROTOCOL *This,
                                    IN UINT16 Attributes)
{
    switch(Attributes)
    {
        case EFI_USB_HC_RESET_GLOBAL:
        case EFI_USB_HC_RESET_HOST_CONTROLLER:
        case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
        case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
            UsbHcStart(USB_HC_FROM_THIS(This));
            return EFI_SUCCESS;
        default:
            return EFI_INVALID_PARAMETER;
    }
}

STATIC EFI_STATUS EFIAPI UsbHcGetState(IN EFI_USB2_HC_PROTOCOL *This,
                                       OUT EFI_USB_HC_STATE *State)
{
    USB_HC  *Hc;

    if(State == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    Hc = USB_HC_FROM_THIS(This);
    *State = Hc->State;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI UsbHcSetState(IN EFI_USB2_HC_PROTOCOL *This,
                                       IN EFI_USB_HC_STATE State)
{
    USB_HC *Hc = USB_HC_FROM_THIS(This);
    switch(State)
    {
        case EfiUsbHcStateHalt:
            MmioWrite64(Hc->Regs + EMEX64_USB_CMD, 0);
            Hc->State = EfiUsbHcStateHalt;
            return EFI_SUCCESS;
        case EfiUsbHcStateOperational:
            MmioWrite64(Hc->Regs + EMEX64_USB_CMD, EMEX64_USB_CMD_RUN);
            Hc->State = EfiUsbHcStateOperational;
            return EFI_SUCCESS;
        case EfiUsbHcStateSuspend:
            return EFI_UNSUPPORTED;
        default:
            return EFI_INVALID_PARAMETER;
    }
}

STATIC EFI_STATUS EFIAPI UsbHcControlTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                              IN UINT8 DeviceAddress,
                                              IN UINT8 DeviceSpeed,
                                              IN UINTN MaximumPacketLength,
                                              IN EFI_USB_DEVICE_REQUEST *Request,
                                              IN EFI_USB_DATA_DIRECTION TransferDirection,
                                              IN OUT VOID *Data,
                                              IN OUT UINTN *DataLength,
                                              IN UINTN TimeOut,
                                              IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                              OUT UINT32 *TransferResult)
{
    if((Request == NULL) || (TransferResult == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    UINTN Length = (DataLength != NULL) ? *DataLength : 0;
    if((TransferDirection != EfiUsbNoData) && ((Data == NULL) || (Length == 0)))
    {
        return EFI_INVALID_PARAMETER;
    }

    if(TransferDirection == EfiUsbNoData)
    {
        Length = 0;
    }

    UINT64 Setup;
    CopyMem(&Setup, Request, sizeof (Setup));
    if(Request->Length > Length)
    {
        return EFI_INVALID_PARAMETER;
    }

    UINTN Actual;
    EFI_STATUS Status = UsbHcRun(USB_HC_FROM_THIS(This), EMEX64_USB_TD_CONTROL, DeviceAddress, 0, Setup, Length != 0 ? Data : NULL, Length, TimeOut, &Actual, TransferResult);
    if(DataLength != NULL)
    {
        *DataLength = Actual;
    }

    return Status;
}

STATIC EFI_STATUS EFIAPI UsbHcBulkTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                           IN UINT8 DeviceAddress,
                                           IN UINT8 EndPointAddress,
                                           IN UINT8 DeviceSpeed,
                                           IN UINTN MaximumPacketLength,
                                           IN UINT8 DataBuffersNumber,
                                           IN OUT VOID *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
                                           IN OUT UINTN *DataLength,
                                           IN OUT UINT8 *DataToggle,
                                           IN UINTN TimeOut,
                                           IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                           OUT UINT32 *TransferResult)
{
    UINTN Actual;
    UINTN Packets;

    if((Data == NULL) || (Data[0] == NULL) || (DataLength == NULL) || (*DataLength == 0) || (DataToggle == NULL) || (TransferResult == NULL) || ((EndPointAddress & 0xF) == 0))
    {
        return EFI_INVALID_PARAMETER;
    }

    EFI_STATUS Status = UsbHcRun(USB_HC_FROM_THIS(This), (EndPointAddress & USB_ENDPOINT_DIR_IN) != 0 ? EMEX64_USB_TD_BULK_IN : EMEX64_USB_TD_BULK_OUT, DeviceAddress, EndPointAddress & 0xF, 0, Data[0], *DataLength, TimeOut, &Actual, TransferResult);
    *DataLength = Actual;

    if((MaximumPacketLength != 0) && (Actual != 0))
    {
        Packets = (Actual + MaximumPacketLength - 1) / MaximumPacketLength;
        *DataToggle = (UINT8)((*DataToggle + Packets) & 1);
    }

    return Status;
}

STATIC EFI_STATUS EFIAPI UsbHcAsyncInterruptTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                                     IN UINT8 DeviceAddress,
                                                     IN UINT8 EndPointAddress,
                                                     IN UINT8 DeviceSpeed,
                                                     IN UINTN MaxiumPacketLength,
                                                     IN BOOLEAN IsNewTransfer,
                                                     IN OUT UINT8 *DataToggle,
                                                     IN UINTN PollingInterval,
                                                     IN UINTN DataLength,
                                                     IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                                     IN EFI_ASYNC_USB_TRANSFER_CALLBACK CallBackFunction,
                                                     IN VOID *Context)
{
    return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS EFIAPI UsbHcSyncInterruptTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                                    IN UINT8 DeviceAddress,
                                                    IN UINT8 EndPointAddress,
                                                    IN UINT8 DeviceSpeed,
                                                    IN UINTN MaximumPacketLength,
                                                    IN OUT VOID *Data,
                                                    IN OUT UINTN *DataLength,
                                                    IN OUT UINT8 *DataToggle,
                                                    IN UINTN TimeOut,
                                                    IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                                    OUT UINT32 *TransferResult)
{
    return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS EFIAPI UsbHcIsochronousTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                                  IN UINT8 DeviceAddress,
                                                  IN UINT8 EndPointAddress,
                                                  IN UINT8 DeviceSpeed,
                                                  IN UINTN MaximumPacketLength,
                                                  IN UINT8 DataBuffersNumber,
                                                  IN OUT VOID *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
                                                  IN UINTN DataLength,
                                                  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                                  OUT UINT32 *TransferResult)
{
    return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS EFIAPI UsbHcAsyncIsochronousTransfer(IN EFI_USB2_HC_PROTOCOL *This,
                                                       IN UINT8 DeviceAddress,
                                                       IN UINT8 EndPointAddress,
                                                       IN UINT8 DeviceSpeed,
                                                       IN UINTN MaximumPacketLength,
                                                       IN UINT8 DataBuffersNumber,
                                                       IN OUT VOID *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
                                                       IN UINTN DataLength,
                                                       IN EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
                                                       IN EFI_ASYNC_USB_TRANSFER_CALLBACK IsochronousCallBack,
                                                       IN VOID *Context)
{
    return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS EFIAPI UsbHcGetRootHubPortStatus(IN EFI_USB2_HC_PROTOCOL *This,
                                                   IN UINT8 PortNumber,
                                                   OUT EFI_USB_PORT_STATUS *PortStatus)
{
    USB_HC *Hc;
    UINT64 Sc;

    Hc = USB_HC_FROM_THIS (This);
    if((PortStatus == NULL) || (PortNumber >= Hc->Ports))
    {
        return EFI_INVALID_PARAMETER;
    }

    Sc = MmioRead64(Hc->Regs + EMEX64_USB_PORTSC (PortNumber));
    PortStatus->PortStatus = USB_PORT_STAT_POWER;
    PortStatus->PortChangeStatus = 0;
    if((Sc & EMEX64_USB_PORTSC_CONNECTED) != 0)
    {
        PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
        switch((Sc >> 8) & 0xF)
        {
            case 1:
                PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
                break;
            case 3:
                PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
                break;
            case 4:
                PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;
                break;
            default:
                break;
        }
    }

    if((Sc & EMEX64_USB_PORTSC_ENABLED) != 0)
    {
        PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
    }

    if((Sc & EMEX64_USB_PORTSC_CHANGE) != 0)
    {
        PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
    }

    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI UsbHcSetRootHubPortFeature(IN EFI_USB2_HC_PROTOCOL *This,
                                                    IN UINT8 PortNumber,
                                                    IN EFI_USB_PORT_FEATURE PortFeature)
{
    USB_HC  *Hc;

    Hc = USB_HC_FROM_THIS(This);
    if(PortNumber >= Hc->Ports)
    {
        return EFI_INVALID_PARAMETER;
    }

    switch(PortFeature)
    {
        case EfiUsbPortReset:
            MmioWrite64(Hc->Regs + EMEX64_USB_PORTCTL(PortNumber), EMEX64_USB_PORTCTL_RESET);
            return EFI_SUCCESS;
        case EfiUsbPortEnable:
        case EfiUsbPortPower:
        case EfiUsbPortSuspend:
        case EfiUsbPortOwner:
            return EFI_SUCCESS;
        default:
            return EFI_INVALID_PARAMETER;
    }
}

STATIC EFI_STATUS EFIAPI UsbHcClearRootHubPortFeature(IN EFI_USB2_HC_PROTOCOL *This,
                                                      IN UINT8 PortNumber,
                                                      IN EFI_USB_PORT_FEATURE  PortFeature)
{
    USB_HC *Hc = USB_HC_FROM_THIS(This);
    if(PortNumber >= Hc->Ports)
    {
        return EFI_INVALID_PARAMETER;
    }

    switch(PortFeature)
    {
        case EfiUsbPortEnable:
            MmioWrite64(Hc->Regs + EMEX64_USB_PORTCTL(PortNumber), EMEX64_USB_PORTCTL_DISABLE);
            return EFI_SUCCESS;
        case EfiUsbPortConnectChange:
            MmioWrite64(Hc->Regs + EMEX64_USB_PORTCTL(PortNumber), EMEX64_USB_PORTCTL_CLEAR_CHANGE);
            return EFI_SUCCESS;
        case EfiUsbPortReset:
        case EfiUsbPortSuspend:
        case EfiUsbPortPower:
        case EfiUsbPortOwner:
        case EfiUsbPortEnableChange:
        case EfiUsbPortSuspendChange:
        case EfiUsbPortOverCurrentChange:
        case EfiUsbPortResetChange:
            return EFI_SUCCESS;
        default:
            return EFI_INVALID_PARAMETER;
    }
}

EFI_STATUS EFIAPI UsbHcDxeInitialize(IN EFI_HANDLE ImageHandle,
                                     IN EFI_SYSTEM_TABLE *SystemTable)
{
    UINT64 Cfg = Emex64PcieFind(EMEX64_USB_DEVICE_ID);
    UINT64 Regs = (Cfg != 0) ? Emex64PcieMapBar0(Cfg) : 0;
    if(Regs == 0)
    {
        DEBUG((DEBUG_INFO, "UsbHcDxe: no USB host controller\n"));
        return EFI_UNSUPPORTED;
    }

    USB_HC *Hc = AllocateZeroPool(sizeof (*Hc));
    if(Hc == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    Hc->Tds = AllocatePages(1);
    if(Hc->Tds == NULL)
    {
        FreePool(Hc);
        return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem(Hc->Tds, EFI_PAGE_SIZE);

    Hc->Signature = USB_HC_SIGNATURE;
    Hc->Regs = Regs;
    Hc->Ports = (UINTN)(MmioRead64(Regs + EMEX64_USB_CAP) & 0xFF);
    if((Hc->Ports == 0) || (Hc->Ports > USB_PORTS))
    {
        Hc->Ports = USB_PORTS;
    }

    Hc->Usb2Hc.GetCapability = UsbHcGetCapability;
    Hc->Usb2Hc.Reset = UsbHcReset;
    Hc->Usb2Hc.GetState = UsbHcGetState;
    Hc->Usb2Hc.SetState = UsbHcSetState;
    Hc->Usb2Hc.ControlTransfer = UsbHcControlTransfer;
    Hc->Usb2Hc.BulkTransfer = UsbHcBulkTransfer;
    Hc->Usb2Hc.AsyncInterruptTransfer = UsbHcAsyncInterruptTransfer;
    Hc->Usb2Hc.SyncInterruptTransfer = UsbHcSyncInterruptTransfer;
    Hc->Usb2Hc.IsochronousTransfer = UsbHcIsochronousTransfer;
    Hc->Usb2Hc.AsyncIsochronousTransfer = UsbHcAsyncIsochronousTransfer;
    Hc->Usb2Hc.GetRootHubPortStatus = UsbHcGetRootHubPortStatus;
    Hc->Usb2Hc.SetRootHubPortFeature = UsbHcSetRootHubPortFeature;
    Hc->Usb2Hc.ClearRootHubPortFeature = UsbHcClearRootHubPortFeature;
    Hc->Usb2Hc.MajorRevision = 0x2;
    Hc->Usb2Hc.MinorRevision = 0x0;

    Hc->DevicePath.Controller.Header.Type = HARDWARE_DEVICE_PATH;
    Hc->DevicePath.Controller.Header.SubType = HW_VENDOR_DP;
    SetDevicePathNodeLength(&Hc->DevicePath.Controller.Header, sizeof (VENDOR_DEVICE_PATH));
    CopyGuid(&Hc->DevicePath.Controller.Guid, &gEmex64UsbDevicePathGuid);
    SetDevicePathEndNode(&Hc->DevicePath.End);

    UsbHcStart(Hc);

    EFI_HANDLE Handle = NULL;
    EFI_STATUS Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDevicePathProtocolGuid, &Hc->DevicePath, &gEfiUsb2HcProtocolGuid, &Hc->Usb2Hc, NULL);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "UsbHcDxe: %u root ports, registers at 0x%Lx\n", Hc->Ports, Regs));
    return Status;
}
