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

#ifndef ETASSEMBLERDRIVER_H
#define ETASSEMBLERDRIVER_H

#include <stddef.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETAssembler/ETAssemblerJob.h>
#include <EmexToolchain/ETAssembler/invocation.h>
#include <EmexToolchain/ETLinker/type.h>
#include <EmexToolchain/ETAssembler/diagnostic/ETAssemblerDiagnosticConsumer.h>

typedef struct __ETAssemblerDriver *ETAssemblerDriverRef;

EFTypeID ETAssemblerDriverGetTypeID(void);

ETAssemblerDriverRef ETAssemblerDriverCreate(EFAllocatorRef allocatorRef, EFArrayRef arguments);
ETAssemblerDriverRef ETAssemblerDriverCreateWithOptions(EFAllocatorRef allocatorRef, EFArrayRef arguments, ETAssemblerDriverOptions driverOptions, ETAssemblerDiagnosticOptions diagnosticOptions);

Boolean ETAssemblerDriverRun(ETAssemblerDriverRef driverRef);

/*
EFArrayRef ETAssemblerDriverGetJobs(ETAssemblerDriverRef driverRef);

EFStringRef ETAssemblerDriverGetOutputPath(ETAssemblerDriverRef driverRef);
ETAssemblerDiagnosticConsumerRef ETAssemblerDriverGetDiagnosticConsumer(ETAssemblerDriverRef driverRef);

ETAssemblerDriverOptions ETAssemblerDriverGetDriverOptions(ETAssemblerDriverRef driverRef);
ETAssemblerDiagnosticOptions ETAssemblerDriverGetDriverOptions(ETAssemblerDriverRef driverRef);
*/

#endif /* ETASSEMBLERDRIVER_H */
