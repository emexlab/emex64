/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "BaseSynchronizationLibInternals.h"

UINT32 EFIAPI InternalSyncIncrement(IN volatile UINT32 *Value)
{
    return __atomic_add_fetch(Value, 1, __ATOMIC_SEQ_CST);
}

UINT32 EFIAPI InternalSyncDecrement(IN volatile UINT32 *Value)
{
    return __atomic_sub_fetch(Value, 1, __ATOMIC_SEQ_CST);
}

UINT16 EFIAPI InternalSyncCompareExchange16(IN volatile UINT16 *Value,
                                            IN UINT16 CompareValue,
                                            IN UINT16 ExchangeValue)
{
    __atomic_compare_exchange_n(Value, &CompareValue, ExchangeValue, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return CompareValue;
}

UINT32 EFIAPI InternalSyncCompareExchange32(IN volatile UINT32 *Value,
                                            IN UINT32 CompareValue,
                                            IN UINT32 ExchangeValue)
{
    __atomic_compare_exchange_n(Value, &CompareValue, ExchangeValue, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return CompareValue;
}

UINT64 EFIAPI InternalSyncCompareExchange64(IN volatile UINT64 *Value,
                                            IN UINT64 CompareValue,
                                            IN UINT64 ExchangeValue)
{
    __atomic_compare_exchange_n(Value, &CompareValue, ExchangeValue, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return CompareValue;
}
