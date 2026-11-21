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
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/ResetNotification.h>
#include <Guid/EventGroup.h>
#include <Emex64Platform.h>

#define NVRAM_SAVE_PERIOD   EFI_TIMER_PERIOD_MILLISECONDS(250)

STATIC UINT64 *mStore;
STATIC UINT64 *mSaved;
STATIC UINTN mWords;
STATIC EFI_EVENT mTimerEvent;
STATIC EFI_EVENT mExitEvent;
STATIC EFI_EVENT mResetEvent;
STATIC VOID *mResetRegistration;

STATIC VOID NvramSave(VOID)
{
    UINTN Index;

    for(Index = 0; Index < mWords; Index++)
    {
        if(mStore[Index] != mSaved[Index])
        {
            MmioWrite64(EMEX64_NVRAM_BASE + Index * sizeof(UINT64), mStore[Index]);
            mSaved[Index] = mStore[Index];
        }
    }
}

STATIC VOID EFIAPI NvramSaveEvent(IN EFI_EVENT Event,
                                  IN VOID *Context)
{
    NvramSave();
}

STATIC VOID EFIAPI NvramResetNotify(IN EFI_RESET_TYPE ResetType,
                                    IN EFI_STATUS ResetStatus,
                                    IN UINTN DataSize,
                                    IN VOID *ResetData OPTIONAL)
{
    if(!EfiAtRuntime())
    {
        NvramSave();
    }
}

STATIC VOID EFIAPI NvramResetProtocolEvent(IN EFI_EVENT Event,
                                           IN VOID *Context)
{
    EFI_RESET_NOTIFICATION_PROTOCOL *ResetNotification;

    if(EFI_ERROR(gBS->LocateProtocol(&gEfiResetNotificationProtocolGuid, NULL, (VOID **)&ResetNotification)))
    {
        return;
    }
    ResetNotification->RegisterResetNotify(ResetNotification, NvramResetNotify);
    gBS->CloseEvent(Event);
}

EFI_STATUS EFIAPI NvramVariableStoreLibConstructor(IN EFI_HANDLE ImageHandle,
                                                   IN EFI_SYSTEM_TABLE *SystemTable)
{
    UINTN StoreSize;
    UINTN Index;
    EFI_STATUS Status;

    if(!PcdGetBool(PcdEmuVariableNvModeEnable))
    {
        return EFI_SUCCESS;
    }

    StoreSize = PcdGet32(PcdVariableStoreSize);
    if(StoreSize == 0 || (StoreSize % sizeof(UINT64)) != 0 || StoreSize > EMEX64_NVRAM_BYTES || MmioRead64(EMEX64_NVRAM_SIZE) < StoreSize)
    {
        DEBUG((DEBUG_ERROR, "NvramVariableStoreLib: the NVRAM doesn't take the variable store, the variables are not kept\n"));
        return EFI_SUCCESS;
    }

    mWords = StoreSize / sizeof(UINT64);
    mStore = AllocateRuntimePages(EFI_SIZE_TO_PAGES(StoreSize));
    mSaved = AllocateRuntimePages(EFI_SIZE_TO_PAGES(StoreSize));
    if(mStore == NULL || mSaved == NULL)
    {
        DEBUG((DEBUG_ERROR, "NvramVariableStoreLib: no memory for the variable store, the variables are not kept\n"));
        return EFI_SUCCESS;
    }

    for(Index = 0; Index < mWords; Index++)
    {
        mSaved[Index] = MmioRead64(EMEX64_NVRAM_BASE + Index * sizeof(UINT64));
    }
    CopyMem(mStore, mSaved, StoreSize);

    Status = PcdSet64S(PcdEmuVariableNvStoreReserved, (UINT64)(UINTN)mStore);
    if(EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "NvramVariableStoreLib: can't hand the store to the variable driver (%r), the variables are not kept\n", Status));
        return EFI_SUCCESS;
    }

    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NvramSaveEvent, NULL, &mTimerEvent);
    if(!EFI_ERROR(Status))
    {
        gBS->SetTimer(mTimerEvent, TimerPeriodic, NVRAM_SAVE_PERIOD);
    }
    gBS->CreateEventEx(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, NvramSaveEvent, NULL, &gEfiEventExitBootServicesGuid, &mExitEvent);
    mResetEvent = EfiCreateProtocolNotifyEvent(&gEfiResetNotificationProtocolGuid, TPL_CALLBACK, NvramResetProtocolEvent, NULL, &mResetRegistration);

    return EFI_SUCCESS;
}
