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

#ifndef ETCOMPILERDRIVER_H
#define ETCOMPILERDRIVER_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETCompiler/ETCompilerJob.h>
#include <EmexToolchain/ETCompiler/ETCompilerOptions.h>
#include <EmexToolchain/ETCompiler/ETCompilerDiagnosticConsumer.h>

typedef struct __ETCompilerDriver *ETCompilerDriverRef;

EFTypeID ETCompilerDriverGetTypeID(void);

ETCompilerDriverRef ETCompilerDriverCreate(EFAllocatorRef allocatorRef, EFArrayRef arguments);
ETCompilerDriverRef ETCompilerDriverCreateWithOptions(EFAllocatorRef allocatorRef, EFArrayRef arguments, ETCompilerDriverOptions driverOptions, ETCompilerDiagnosticOptions diagnosticOptions);

Boolean ETCompilerDriverRun(ETCompilerDriverRef driverRef);

#endif /* ETCOMPILERDRIVER_H */
