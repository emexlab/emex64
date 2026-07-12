/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef E64MMIOBUS_H
#define E64MMIOBUS_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/vm/E64MMIORegion.h>
#if ET_PRIVATE
#include <EmexToolchain/vm/__E64MMIOBus.h>
#endif /* ET_PRIVATE */

typedef EFObjectRef E64MMIOBusRef;

EFTypeID E64MMIOBusGetTypeID(void);

E64MMIOBusRef E64MMIOBusCreate(EFAllocatorRef allocatorRef);

Boolean E64MMIOBusRegisterRegion(E64MMIOBusRef MMIOBusRef, E64MMIORegionRef MMIORegionRef);
E64MMIORegionRef E64MMIOBusGetRegionForAddress(E64MMIOBusRef MMIOBusRef, UInt64 addr);

#endif /* E64MMIOBUS_H */
