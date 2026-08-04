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

#ifndef EMEX64VM_CORE_H
#define EMEX64VM_CORE_H

#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Type.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/VM/__E64Core.h>
#endif /* ET_PRIVATE */

#define E64VM_ISA_MIN_VERSION 0
#define E64VM_ISA_MAX_VERSION 15

typedef struct __E64Core *E64CoreRef;
typedef struct __E64Machine *E64MachineRef;

EFTypeID E64CoreGetTypeID(void);

E64CoreRef E64CoreCreateWithMachine(EFAllocatorRef allocatorRef, E64MachineRef machineRef);
E64Exception E64CoreExecute(E64CoreRef coreRef);
void E64CoreTerminate(E64CoreRef coreRef);

UInt64 E64CoreGetValueFromRegister(E64CoreRef coreRef, E64Register reg);
void E64CoreSetRegisterWithValue(E64CoreRef coreRef, E64Register reg, UInt64 value);

E64Exception E64CoreGetException(E64CoreRef coreRef);

#endif /* EMEX64VM_CORE_H */
