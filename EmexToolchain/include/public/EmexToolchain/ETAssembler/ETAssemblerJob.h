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

#ifndef ETASSEMBLERJOB_H
#define ETASSEMBLERJOB_H

#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETLinker/type.h>

typedef enum: UInt8 {
    kETAssemblerJobTypeUnknown,
    kETAssemblerJobTypeAssembler,
    kETAssemblerJobTypeDriver,
    kETAssemblerJobTypeLinker
} ETAssemblerJobType;

typedef struct __ETAssemblerJob *ETAssemblerJobRef;

EFTypeID ETAssemblerJobGetTypeID(void);

ETAssemblerJobRef ETAssemblerJobCreate(EFAllocatorRef allocatorRef, ETAssemblerJobType type, EFStringRef command, EFArrayRef arguments);

ETAssemblerJobType ETAssemblerJobGetType(ETAssemblerJobRef jobRef);
EFStringRef ETAssemblerJobGetCommand(ETAssemblerJobRef jobRef);
EFArrayRef ETAssemblerJobGetArguments(ETAssemblerJobRef jobRef);

#endif /* ETASSEMBLERJOB_H */
