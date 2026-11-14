/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/PrePiHobListPointerLib.h>

STATIC VOID *mHobList;

VOID *EFIAPI PrePeiGetHobList(VOID)
{
    return mHobList;
}

EFI_STATUS EFIAPI PrePeiSetHobList(IN VOID *HobList)
{
    mHobList = HobList;
    return EFI_SUCCESS;
}
