/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EMEX64_PCIE_H_
#define EMEX64_PCIE_H_

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Emex64Platform.h>

STATIC UINT64 Emex64PcieFind(IN UINT16 DeviceId)
{
    UINT64 Ecam;
    UINT64 Cfg;
    UINT64 Id;
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
        if(((Id & 0xFFFF) == EMEX64_GPU_VENDOR_ID) && (((Id >> 16) & 0xFFFF) == DeviceId))
        {
            return Cfg;
        }
    }

    return 0;
}

STATIC BOOLEAN Emex64PcieBarTaken(IN UINT64 Self,
                                  IN UINT64 Base,
                                  IN UINT64 Size,
                                  OUT UINT64 *Next)
{
    UINT64 Ecam;
    UINT64 Cfg;
    UINT64 Bar;
    UINT64 BarSize;
    UINTN Dev;
    UINTN N;

    Ecam = MmioRead64(EMEX64_PCIE_RC_ECAM);
    for(Dev = 0; Dev < 32; Dev++)
    {
        Cfg = Ecam + LShiftU64 (Dev, 16);
        if((Cfg == Self) || ((MmioRead64(Cfg + EMEX64_PCIE_CFG_ID) & 0xFFFF) != EMEX64_GPU_VENDOR_ID))
        {
            continue;
        }

        if((MmioRead64(Cfg + EMEX64_PCIE_CFG_CONTROL) & EMEX64_PCIE_CONTROL_MEM) == 0)
        {
            continue;
        }

        for(N = 0; N < 6; N++)
        {
            BarSize = MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR_SIZE(N));
            Bar = MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR(N));
            if((BarSize != 0) && (Bar < Base + Size) && (Base < Bar + BarSize))
            {
                *Next = Bar + BarSize;
                return TRUE;
            }
        }
    }

    return FALSE;
}

STATIC UINT64 Emex64PcieMapBar0(IN UINT64 Cfg)
{
    UINT64 Size;
    UINT64 Base;
    UINT64 Next;
    UINT64 Limit;

    Size = MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR_SIZE(0));
    if(Size == 0)
    {
        return 0;
    }

    if((MmioRead64(Cfg + EMEX64_PCIE_CFG_CONTROL) & EMEX64_PCIE_CONTROL_MEM) == 0)
    {
        MmioWrite64(Cfg + EMEX64_PCIE_CFG_CONTROL, 0);
        Limit = MmioRead64(EMEX64_PCIE_RC_MMIO_BASE) + MmioRead64(EMEX64_PCIE_RC_MMIO_SIZE);
        Base = (MmioRead64(EMEX64_PCIE_RC_MMIO_BASE) + Size - 1) & ~(Size - 1);
        while(Emex64PcieBarTaken(Cfg, Base, Size, &Next))
        {
            Base = (Next + Size - 1) & ~(Size - 1);
        }

        if(Base + Size > Limit)
        {
            return 0;
        }

        MmioWrite64(Cfg + EMEX64_PCIE_CFG_BAR(0), Base);
    }

    MmioWrite64(Cfg + EMEX64_PCIE_CFG_CONTROL, EMEX64_PCIE_CONTROL_MEM | EMEX64_PCIE_CONTROL_MASTER);
    return MmioRead64(Cfg + EMEX64_PCIE_CFG_BAR(0));
}

#endif /* EMEX64_PCIE_H_ */
