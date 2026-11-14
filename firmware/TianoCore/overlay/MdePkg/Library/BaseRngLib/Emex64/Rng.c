/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>

#include "BaseRngLibInternals.h"

EFI_STATUS EFIAPI BaseRngLibConstructor(VOID)
{
    return EFI_SUCCESS;
}

BOOLEAN EFIAPI ArchGetRandomNumber16(OUT UINT16 *Rand)
{
    UINT64  Value;
    
    if(!ArchGetRandomNumber64(&Value))
    {
        return FALSE;
    }

    *Rand = (UINT16)Value;
    return TRUE;
}

BOOLEAN EFIAPI ArchGetRandomNumber32(OUT UINT32 *Rand)
{
    UINT64 Value;
    
    if(!ArchGetRandomNumber64(&Value))
    {
        return FALSE;
    }

    *Rand = (UINT32)Value;
    return TRUE;
}

BOOLEAN EFIAPI ArchGetRandomNumber64(OUT UINT64 *Rand)
{
    UINT64 Value;
    __asm__ __volatile__("rdrnd %0" : "=r" (Value));
    *Rand = Value;
    return TRUE;
}

BOOLEAN EFIAPI ArchIsRngSupported(VOID)
{
    return TRUE;
}

RETURN_STATUS EFIAPI GetRngGuid(GUID *RngGuid)
{
    if(RngGuid == NULL)
    {
        return RETURN_INVALID_PARAMETER;
    }

    RngGuid->Data1 = gEfiRngAlgorithmRaw.Data1;
    RngGuid->Data2 = gEfiRngAlgorithmRaw.Data2;
    RngGuid->Data3 = gEfiRngAlgorithmRaw.Data3;
    for(UINTN Index = 0; Index < sizeof(RngGuid->Data4); Index++)
    {
        RngGuid->Data4[Index] = gEfiRngAlgorithmRaw.Data4[Index];
    }

    return RETURN_SUCCESS;
}
