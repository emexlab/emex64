/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <Guid/SerialPortLibVendor.h>
#include <Guid/TtyTerm.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootLogoLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GraphicsOutput.h>
#include <Emex64Platform.h>

#define DP_NODE_LEN(Type)                               \
{                                                       \
    (UINT8)sizeof (Type), (UINT8)(sizeof (Type) >> 8)   \
}

#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH        Vendor;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_VENDOR_DEVICE;

typedef struct {
  VENDOR_DEVICE_PATH        SerialDxe;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        Terminal;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_SERIAL_CONSOLE;
#pragma pack ()

STATIC BOOLEAN mDisplayAvailable;
STATIC BOOLEAN mSetupHintShown;

STATIC PLATFORM_VENDOR_DEVICE mDisplay = {
    {
        { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DP_NODE_LEN(VENDOR_DEVICE_PATH) },
        { 0 }
    },
    {
        END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, DP_NODE_LEN(EFI_DEVICE_PATH_PROTOCOL)
    }
};

STATIC PLATFORM_VENDOR_DEVICE  mKeyboard = {
    {
        { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DP_NODE_LEN(VENDOR_DEVICE_PATH) },
        { 0 }
    },
    {
        END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, DP_NODE_LEN(EFI_DEVICE_PATH_PROTOCOL)
    }
};

STATIC PLATFORM_SERIAL_CONSOLE mSerial = {
    {
        { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DP_NODE_LEN(VENDOR_DEVICE_PATH) },
        EDKII_SERIAL_PORT_LIB_VENDOR_GUID
    },
    {
        { MESSAGING_DEVICE_PATH, MSG_UART_DP, DP_NODE_LEN(UART_DEVICE_PATH) },
        0,
        FixedPcdGet64(PcdUartDefaultBaudRate),
        FixedPcdGet8(PcdUartDefaultDataBits),
        FixedPcdGet8(PcdUartDefaultParity),
        FixedPcdGet8(PcdUartDefaultStopBits)
    },
    {
        { MESSAGING_DEVICE_PATH, MSG_VENDOR_DP, DP_NODE_LEN(VENDOR_DEVICE_PATH) },
        EFI_TTY_TERM_GUID
    },
    {
        END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, DP_NODE_LEN(EFI_DEVICE_PATH_PROTOCOL)
    }
};

STATIC VOID RegisterSetupKeys(VOID)
{
    EFI_STATUS Status;
    EFI_INPUT_KEY Enter;
    EFI_INPUT_KEY F2;
    EFI_INPUT_KEY Esc;
    EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;

    Enter.ScanCode = SCAN_NULL;
    Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
    Status = EfiBootManagerRegisterContinueKeyOption(0, &Enter, NULL);
    ASSERT_EFI_ERROR(Status);

    Status = EfiBootManagerGetBootManagerMenu(&BootManagerMenu);
    if(EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "PlatformBm: no setup utility in the firmware (%r)\n", Status));
        return;
    }

    F2.ScanCode = SCAN_F2;
    F2.UnicodeChar = CHAR_NULL;
    Esc.ScanCode = SCAN_ESC;
    Esc.UnicodeChar = CHAR_NULL;

    Status = EfiBootManagerAddKeyOptionVariable(NULL, (UINT16)BootManagerMenu.OptionNumber, 0, &F2, NULL);
    ASSERT(Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);
    Status = EfiBootManagerAddKeyOptionVariable(NULL, (UINT16)BootManagerMenu.OptionNumber, 0, &Esc, NULL);
    ASSERT(Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED);

    EfiBootManagerFreeLoadOption(&BootManagerMenu);
}

VOID EFIAPI PlatformBootManagerBeforeConsole(VOID)
{
    EFI_STATUS Status;
    VOID *Gop;

    EfiEventGroupSignal(&gEfiEndOfDxeEventGroupGuid);
    EfiBootManagerDispatchDeferredImages();

    CopyGuid(&mDisplay.Vendor.Guid, &gEmex64DisplayDevicePathGuid);
    CopyGuid(&mKeyboard.Vendor.Guid, &gEmex64KeyboardDevicePathGuid);

    mDisplayAvailable = !EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, &Gop));

    EfiBootManagerUpdateConsoleVariable(ConIn, (EFI_DEVICE_PATH_PROTOCOL *)&mKeyboard, NULL);
    if(mDisplayAvailable)
    {
        EfiBootManagerUpdateConsoleVariable(ConIn, NULL, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial);
        EfiBootManagerUpdateConsoleVariable(ConOut, (EFI_DEVICE_PATH_PROTOCOL *)&mDisplay, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial);
        EfiBootManagerUpdateConsoleVariable(ErrOut, (EFI_DEVICE_PATH_PROTOCOL *)&mDisplay, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial);
    }
    else
    {
        DEBUG((DEBUG_INFO, "PlatformBm: no display, the console is the serial port\n"));
        EfiBootManagerUpdateConsoleVariable(ConIn, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial, NULL);
        EfiBootManagerUpdateConsoleVariable(ConOut, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial, (EFI_DEVICE_PATH_PROTOCOL *)&mDisplay);
        EfiBootManagerUpdateConsoleVariable(ErrOut, (EFI_DEVICE_PATH_PROTOCOL *)&mSerial, (EFI_DEVICE_PATH_PROTOCOL *)&mDisplay);

        Status = PcdSet32S(PcdConOutColumn, 80);
        ASSERT_EFI_ERROR(Status);
        Status = PcdSet32S(PcdConOutRow, 25);
        ASSERT_EFI_ERROR(Status);
    }

    RegisterSetupKeys();
}

STATIC BOOLEAN IsUsbOption(IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Option)
{
    EFI_DEVICE_PATH_PROTOCOL  *Node;
    for(Node = Option->FilePath; !IsDevicePathEnd (Node); Node = NextDevicePathNode (Node))
    {
        if((DevicePathType (Node) == MESSAGING_DEVICE_PATH) && ((DevicePathSubType (Node) == MSG_USB_DP) || (DevicePathSubType (Node) == MSG_USB_CLASS_DP) || (DevicePathSubType (Node) == MSG_USB_WWID_DP)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

STATIC INTN EFIAPI CompareUsbFirst(IN CONST VOID *Left,
                                   IN CONST VOID *Right)
{
    CONST EFI_BOOT_MANAGER_LOAD_OPTION *A;
    CONST EFI_BOOT_MANAGER_LOAD_OPTION *B;
    BOOLEAN UsbA;
    BOOLEAN UsbB;

    A = Left;
    B = Right;
    UsbA = IsUsbOption(A);
    UsbB = IsUsbOption(B);
    if(UsbA != UsbB)
    {
        return UsbA ? -1 : 1;
    }

    return (INTN)A->OptionNumber - (INTN)B->OptionNumber;
}

VOID EFIAPI PlatformBootManagerAfterConsole(VOID)
{
    BootLogoEnableLogo();

    EfiBootManagerConnectAll();
    EfiBootManagerRefreshAllBootOption();
    EfiBootManagerSortLoadOptionVariable(LoadOptionTypeBoot, CompareUsbFirst);
}

VOID EFIAPI PlatformBootManagerWaitCallback(UINT16 TimeoutRemain)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Black;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION White;
    UINT16 Timeout;
    
    Timeout = PcdGet16(PcdPlatformBootTimeOut);
    if(Timeout == 0)
    {
        return;
    }

    if(!mDisplayAvailable)
    {
        if(!mSetupHintShown)
        {
            mSetupHintShown = TRUE;
            Print(L"Press ESC or F2 to enter setup\r\n");
        }
        return;
    }

    Black.Raw = 0x00000000;
    White.Raw = 0x00FFFFFF;

    BootLogoUpdateProgress(White.Pixel, Black.Pixel, L"Press ESC or F2 to enter setup", White.Pixel, (Timeout - TimeoutRemain) * 100 / Timeout, 0);
}

VOID EFIAPI PlatformBootManagerUnableToBoot(VOID)
{
    EFI_STATUS Status;
    EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;
    Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
    if(EFI_ERROR (Status))
    {
        return;
    }
    for(;;)
    {
        EfiBootManagerBoot(&BootManagerMenu);
    }
}
