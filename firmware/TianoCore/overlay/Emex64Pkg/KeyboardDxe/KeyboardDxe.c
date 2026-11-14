/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Emex64Platform.h>

#define POLL_PERIOD             (20 * 10000)
#define QUEUE_SIZE              32

#pragma pack (1)
typedef struct {
    VENDOR_DEVICE_PATH          Vendor;
    EFI_DEVICE_PATH_PROTOCOL    End;
} KEYBOARD_DEVICE_PATH;
#pragma pack ()

typedef struct {
    LIST_ENTRY                  Link;
    EFI_KEY_DATA                KeyData;
    EFI_KEY_NOTIFY_FUNCTION     Function;
} KEY_NOTIFY;

STATIC KEYBOARD_DEVICE_PATH  mDevicePath = {
    {
        { HARDWARE_DEVICE_PATH, HW_VENDOR_DP, { sizeof (VENDOR_DEVICE_PATH), 0 } },
        { 0 }
    },
    {
        END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
};

STATIC EFI_SIMPLE_TEXT_INPUT_PROTOCOL       mTextIn;
STATIC EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    mTextInEx;

STATIC EFI_KEY_DATA     mQueue[QUEUE_SIZE];
STATIC UINTN            mQueueHead;
STATIC UINTN            mQueueTail;

STATIC EFI_KEY_DATA     mNotifyQueue[QUEUE_SIZE];
STATIC UINTN            mNotifyHead;
STATIC UINTN            mNotifyTail;
STATIC EFI_EVENT        mNotifyEvent;
STATIC LIST_ENTRY       mNotifyList = INITIALIZE_LIST_HEAD_VARIABLE (mNotifyList);

STATIC BOOLEAN          mExtended;
STATIC UINTN            mSkip;
STATIC BOOLEAN          mLeftShift, mRightShift, mLeftCtrl, mRightCtrl, mLeftAlt, mRightAlt, mLeftLogo, mRightLogo, mMenu;
STATIC BOOLEAN          mCapsLock, mNumLock, mScrollLock;

STATIC CONST CHAR16  mPlain[] = {
    0, 0, L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'0', L'-', L'=', CHAR_BACKSPACE, CHAR_TAB,
    L'q', L'w', L'e', L'r', L't', L'y', L'u', L'i', L'o', L'p', L'[', L']', CHAR_CARRIAGE_RETURN, 0, L'a', L's',
    L'd', L'f', L'g', L'h', L'j', L'k', L'l', L';', L'\'', L'`', 0, L'\\', L'z', L'x', L'c', L'v',
    L'b', L'n', L'm', L',', L'.', L'/', 0, L'*', 0, L' ', 0
};

STATIC CONST CHAR16  mShifted[] = {
    0, 0, L'!', L'@', L'#', L'$', L'%', L'^', L'&', L'*', L'(', L')', L'_', L'+', CHAR_BACKSPACE, CHAR_TAB,
    L'Q', L'W', L'E', L'R', L'T', L'Y', L'U', L'I', L'O', L'P', L'{', L'}', CHAR_CARRIAGE_RETURN, 0, L'A', L'S',
    L'D', L'F', L'G', L'H', L'J', L'K', L'L', L':', L'"', L'~', 0, L'|', L'Z', L'X', L'C', L'V',
    L'B', L'N', L'M', L'<', L'>', L'?', 0, L'*', 0, L' ', 0
};

STATIC CONST CHAR16  mKeypadChar[] = {
    L'7', L'8', L'9', L'-', L'4', L'5', L'6', L'+', L'1', L'2', L'3', L'0', L'.'
};

STATIC CONST UINT16  mKeypadScan[] = {
    SCAN_HOME, SCAN_UP, SCAN_PAGE_UP, SCAN_NULL, SCAN_LEFT, SCAN_NULL, SCAN_RIGHT, SCAN_NULL,
    SCAN_END, SCAN_DOWN, SCAN_PAGE_DOWN, SCAN_INSERT, SCAN_DELETE
};

STATIC VOID FillState(OUT EFI_KEY_STATE *State)
{
    State->KeyShiftState = EFI_SHIFT_STATE_VALID;
    State->KeyShiftState |= mLeftShift ? EFI_LEFT_SHIFT_PRESSED : 0;
    State->KeyShiftState |= mRightShift ? EFI_RIGHT_SHIFT_PRESSED : 0;
    State->KeyShiftState |= mLeftCtrl ? EFI_LEFT_CONTROL_PRESSED : 0;
    State->KeyShiftState |= mRightCtrl ? EFI_RIGHT_CONTROL_PRESSED : 0;
    State->KeyShiftState |= mLeftAlt ? EFI_LEFT_ALT_PRESSED : 0;
    State->KeyShiftState |= mRightAlt ? EFI_RIGHT_ALT_PRESSED : 0;
    State->KeyShiftState |= mLeftLogo ? EFI_LEFT_LOGO_PRESSED : 0;
    State->KeyShiftState |= mRightLogo ? EFI_RIGHT_LOGO_PRESSED : 0;
    State->KeyShiftState |= mMenu ? EFI_MENU_KEY_PRESSED : 0;
    State->KeyToggleState = EFI_TOGGLE_STATE_VALID;
    State->KeyToggleState |= mCapsLock ? EFI_CAPS_LOCK_ACTIVE : 0;
    State->KeyToggleState |= mNumLock ? EFI_NUM_LOCK_ACTIVE : 0;
    State->KeyToggleState |= mScrollLock ? EFI_SCROLL_LOCK_ACTIVE : 0;
}

STATIC BOOLEAN KeyMatches(IN EFI_KEY_DATA *Registered,
                          IN EFI_KEY_DATA *Key)
{
    if((Registered->Key.ScanCode != Key->Key.ScanCode) || (Registered->Key.UnicodeChar != Key->Key.UnicodeChar))
    {
        return FALSE;
    }

    if(((Registered->KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) && (Registered->KeyState.KeyShiftState != Key->KeyState.KeyShiftState))
    {
        return FALSE;
    }

    if(((Registered->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != 0) && (Registered->KeyState.KeyToggleState != Key->KeyState.KeyToggleState))
    {
        return FALSE;
    }

    return TRUE;
}

STATIC VOID QueueKey(IN UINT16 ScanCode,
                     IN CHAR16 Char)
{
    EFI_KEY_DATA Key;
    LIST_ENTRY *Link;
    KEY_NOTIFY *Notify;
    UINTN Next;

    Key.Key.ScanCode = ScanCode;
    Key.Key.UnicodeChar = Char;
    FillState(&Key.KeyState);

    for(Link = GetFirstNode(&mNotifyList); !IsNull(&mNotifyList, Link); Link = GetNextNode(&mNotifyList, Link))
    {
        Notify = BASE_CR(Link, KEY_NOTIFY, Link);
        if(KeyMatches(&Notify->KeyData, &Key))
        {
            Next = (mNotifyTail + 1) % QUEUE_SIZE;
            if(Next != mNotifyHead)
            {
                mNotifyQueue[mNotifyTail] = Key;
                mNotifyTail = Next;
                gBS->SignalEvent (mNotifyEvent);
            }
            break;
        }
    }

    Next = (mQueueTail + 1) % QUEUE_SIZE;
    if(Next == mQueueHead)
    {
        return;
    }

    mQueue[mQueueTail] = Key;
    mQueueTail = Next;
}

STATIC VOID HandleScanCode(IN UINT8 Code)
{
    BOOLEAN Break;
    BOOLEAN Extended;
    BOOLEAN Shift;
    BOOLEAN Ctrl;
    CHAR16 Char;
    UINT8 Make;

    if(mSkip > 0)
    {
        mSkip--;
        return;
    }

    if(Code == 0xE0)
    {
        mExtended = TRUE;
        return;
    }

    if(Code == 0xE1)
    {
        mSkip = 2;
        return;
    }

    Extended = mExtended;
    mExtended = FALSE;
    Break = (Code & 0x80) != 0;
    Make = Code & 0x7F;

    if(Extended)
    {
        switch(Make)
        {
            case 0x2A:
            case 0x36:
                return;
            case 0x1D:
                mRightCtrl = !Break;
                return;
            case 0x38:
                mRightAlt = !Break;
                return;
            case 0x5B:
                mLeftLogo = !Break;
                return;
            case 0x5C:
                mRightLogo = !Break;
                return;
            case 0x5D:
                mMenu = !Break;
                return;
            default:
                break;
        }

        if(Break)
        {
            return;
        }

        switch(Make)
        {
            case 0x1C:
                QueueKey(SCAN_NULL, CHAR_CARRIAGE_RETURN);
                return;
            case 0x35:
                QueueKey(SCAN_NULL, L'/');
                return;
            case 0x37:
                QueueKey(SCAN_NULL, 0);
                return;
            case 0x47:
                QueueKey(SCAN_HOME, 0);
                return;
            case 0x48:
                QueueKey(SCAN_UP, 0);
                return;
            case 0x49:
                QueueKey(SCAN_PAGE_UP, 0);
                return;
            case 0x4B:
                QueueKey(SCAN_LEFT, 0);
                return;
            case 0x4D:
                QueueKey(SCAN_RIGHT, 0);
                return;
            case 0x4F:
                QueueKey(SCAN_END, 0);
                return;
            case 0x50:
                QueueKey(SCAN_DOWN, 0);
                return;
            case 0x51:
                QueueKey(SCAN_PAGE_DOWN, 0);
                return;
            case 0x52:
                QueueKey(SCAN_INSERT, 0);
                return;
            case 0x53:
                QueueKey(SCAN_DELETE, 0);
                return;
            default:
                return;
        }
    }

    switch(Make)
    {
        case 0x2A:
            mLeftShift = !Break;
            return;
        case 0x36:
            mRightShift = !Break;
            return;
        case 0x1D:
            mLeftCtrl = !Break;
            return;
        case 0x38:
            mLeftAlt = !Break;
            return;
        default:
            break;
    }

    if(Break)
    {
        return;
    }

    switch(Make)
    {
        case 0x3A:
            mCapsLock = !mCapsLock;
            return;
        case 0x45:
            mNumLock = !mNumLock;
            return;
        case 0x46:
            mScrollLock = !mScrollLock;
            return;
        case 0x01:
            QueueKey(SCAN_ESC, 0);
            return;
        case 0x57:
            QueueKey(SCAN_F11, 0);
            return;
        case 0x58:
            QueueKey(SCAN_F12, 0);
            return;
        default:
            break;
    }

    if((Make >= 0x3B) && (Make <= 0x44))
    {
        QueueKey((UINT16)(SCAN_F1 + (Make - 0x3B)), 0);
        return;
    }

    if((Make >= 0x47) && (Make <= 0x53))
    {
        if(mNumLock && !(mLeftShift || mRightShift))
        {
            QueueKey(SCAN_NULL, mKeypadChar[Make - 0x47]);
        }
        else if(mKeypadScan[Make - 0x47] != SCAN_NULL)
        {
            QueueKey(mKeypadScan[Make - 0x47], 0);
        }
        else
        {
            QueueKey(SCAN_NULL, mKeypadChar[Make - 0x47]);
        }
        return;
    }

    if(Make >= ARRAY_SIZE(mPlain))
    {
        return;
    }

    Shift = mLeftShift || mRightShift;
    Ctrl = mLeftCtrl || mRightCtrl;
    Char = Shift ? mShifted[Make] : mPlain[Make];
    if(Char == 0)
    {
        return;
    }

    if(mCapsLock && (((Char >= L'a') && (Char <= L'z')) || ((Char >= L'A') && (Char <= L'Z'))))
    {
        Char ^= 0x20;
    }

    if(Ctrl && (((Char >= L'a') && (Char <= L'z')) || ((Char >= L'A') && (Char <= L'Z'))))
    {
        Char = (CHAR16)((Char & 0x1F));
    }

    QueueKey(SCAN_NULL, Char);
}

STATIC VOID PollKeyboard(VOID)
{
    UINT64 Status;
    UINT8 Code;
    EFI_TPL OldTpl;

    OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
    for(;;)
    {
        Status = MmioRead64(EMEX64_8042_STATUS);
        if((Status & EMEX64_8042_STATUS_OBF) == 0)
        {
            break;
        }

        Code = (UINT8)MmioRead64(EMEX64_8042_DATA);
        if((Status & EMEX64_8042_STATUS_AUX) == 0)
        {
            HandleScanCode(Code);
        }
    }

    gBS->RestoreTPL(OldTpl);
}

STATIC VOID EFIAPI PollTimer(IN EFI_EVENT Event,
                             IN VOID *Context)
{
    PollKeyboard();
}

STATIC VOID EFIAPI NotifyKeys(IN EFI_EVENT Event,
                              IN VOID *Context)
{
    EFI_KEY_DATA Key;
    LIST_ENTRY *Link;
    KEY_NOTIFY *Notify;
    EFI_TPL OldTpl;

    for(;;)
    {
        OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
        if(mNotifyHead == mNotifyTail)
        {
            gBS->RestoreTPL (OldTpl);
            return;
        }

        Key = mNotifyQueue[mNotifyHead];
        mNotifyHead = (mNotifyHead + 1) % QUEUE_SIZE;
        gBS->RestoreTPL(OldTpl);
        for(Link = GetFirstNode(&mNotifyList); !IsNull(&mNotifyList, Link); Link = GetNextNode(&mNotifyList, Link))
        {
            Notify = BASE_CR(Link, KEY_NOTIFY, Link);
            if(KeyMatches(&Notify->KeyData, &Key))
            {
                Notify->Function (&Key);
            }
        }
    }
}

STATIC EFI_STATUS ReadKey(OUT EFI_KEY_DATA *KeyData)
{
    EFI_TPL OldTpl;
    EFI_STATUS Status;

    PollKeyboard();

    OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
    if(mQueueHead == mQueueTail)
    {
        Status = EFI_NOT_READY;
    }
    else
    {
        *KeyData = mQueue[mQueueHead];
        mQueueHead = (mQueueHead + 1) % QUEUE_SIZE;
        Status = EFI_SUCCESS;
    }

    gBS->RestoreTPL (OldTpl);
    return Status;
}

STATIC EFI_STATUS EFIAPI KeyboardReset(IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
                                       IN BOOLEAN ExtendedVerification)
{
    EFI_TPL OldTpl;
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    mQueueHead = mQueueTail = 0;
    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI KeyboardReadKeyStroke(IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
                                               OUT EFI_INPUT_KEY *Key)
{
    EFI_KEY_DATA KeyData;
    EFI_STATUS Status;

    if(Key == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    do {
        Status = ReadKey(&KeyData);
        if(EFI_ERROR(Status))
        {
            return Status;
        }
    } while((KeyData.Key.ScanCode == SCAN_NULL) && (KeyData.Key.UnicodeChar == 0));

    *Key = KeyData.Key;
    return EFI_SUCCESS;
}

STATIC VOID EFIAPI KeyboardWaitForKey(IN EFI_EVENT Event,
                                      IN VOID *Context)
{
    PollKeyboard();
    if(mQueueHead != mQueueTail)
    {
        gBS->SignalEvent (Event);
    }
}

STATIC EFI_STATUS EFIAPI KeyboardResetEx(IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
                                         IN BOOLEAN ExtendedVerification)
{
    return KeyboardReset(&mTextIn, ExtendedVerification);
}

STATIC EFI_STATUS EFIAPI KeyboardReadKeyStrokeEx(IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
                                                 OUT EFI_KEY_DATA *KeyData)
{
    if(KeyData == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    return ReadKey(KeyData);
}

STATIC EFI_STATUS EFIAPI KeyboardSetState(IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
                                          IN EFI_KEY_TOGGLE_STATE *KeyToggleState)
{
    if(KeyToggleState == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    if((*KeyToggleState & EFI_TOGGLE_STATE_VALID) == 0)
    {
        return EFI_UNSUPPORTED;
    }

    mCapsLock = (*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) != 0;
    mNumLock = (*KeyToggleState & EFI_NUM_LOCK_ACTIVE) != 0;
    mScrollLock = (*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) != 0;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI KeyboardRegisterKeyNotify(IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
                                                   IN EFI_KEY_DATA *KeyData,
                                                   IN EFI_KEY_NOTIFY_FUNCTION KeyNotificationFunction,
                                                   OUT VOID **NotifyHandle)
{
    KEY_NOTIFY *Notify;
    LIST_ENTRY *Link;

    if((KeyData == NULL) || (KeyNotificationFunction == NULL) || (NotifyHandle == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    for(Link = GetFirstNode(&mNotifyList); !IsNull(&mNotifyList, Link); Link = GetNextNode(&mNotifyList, Link))
    {
        Notify = BASE_CR (Link, KEY_NOTIFY, Link);
        if((Notify->Function == KeyNotificationFunction) && KeyMatches (&Notify->KeyData, KeyData))
        {
            *NotifyHandle = Notify;
            return EFI_SUCCESS;
        }
    }

    Notify = AllocateZeroPool(sizeof (*Notify));
    if(Notify == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    Notify->KeyData = *KeyData;
    Notify->Function = KeyNotificationFunction;
    InsertTailList(&mNotifyList, &Notify->Link);
    *NotifyHandle = Notify;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI KeyboardUnregisterKeyNotify(IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
                                                     IN VOID *NotificationHandle)
{
    LIST_ENTRY *Link;

    if(NotificationHandle == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    for(Link = GetFirstNode(&mNotifyList); !IsNull(&mNotifyList, Link); Link = GetNextNode(&mNotifyList, Link))
    {
        if(Link == &((KEY_NOTIFY *)NotificationHandle)->Link)
        {
            RemoveEntryList(Link);
            FreePool(NotificationHandle);
            return EFI_SUCCESS;
        }
    }

    return EFI_INVALID_PARAMETER;
}

EFI_STATUS EFIAPI KeyboardDxeInitialize(IN EFI_HANDLE ImageHandle,
                                        IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_HANDLE Handle;
    EFI_EVENT Timer;

    mTextIn.Reset = KeyboardReset;
    mTextIn.ReadKeyStroke = KeyboardReadKeyStroke;
    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT, TPL_NOTIFY, KeyboardWaitForKey, NULL, &mTextIn.WaitForKey);
    ASSERT_EFI_ERROR(Status);

    mTextInEx.Reset = KeyboardResetEx;
    mTextInEx.ReadKeyStrokeEx = KeyboardReadKeyStrokeEx;
    mTextInEx.SetState = KeyboardSetState;
    mTextInEx.RegisterKeyNotify = KeyboardRegisterKeyNotify;
    mTextInEx.UnregisterKeyNotify = KeyboardUnregisterKeyNotify;
    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT, TPL_NOTIFY, KeyboardWaitForKey, NULL, &mTextInEx.WaitForKeyEx);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyKeys, NULL, &mNotifyEvent);
    ASSERT_EFI_ERROR(Status);

    while((MmioRead64 (EMEX64_8042_STATUS) & EMEX64_8042_STATUS_OBF) != 0)
    {
        MmioRead64(EMEX64_8042_DATA);
    }

    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY, PollTimer, NULL, &Timer);
    ASSERT_EFI_ERROR(Status);
    Status = gBS->SetTimer(Timer, TimerPeriodic, POLL_PERIOD);
    ASSERT_EFI_ERROR(Status);

    CopyGuid(&mDevicePath.Vendor.Guid, &gEmex64KeyboardDevicePathGuid);
    Handle = NULL;
    return gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDevicePathProtocolGuid, &mDevicePath, &gEfiSimpleTextInProtocolGuid, &mTextIn, &gEfiSimpleTextInputExProtocolGuid, &mTextInEx, NULL);
}
