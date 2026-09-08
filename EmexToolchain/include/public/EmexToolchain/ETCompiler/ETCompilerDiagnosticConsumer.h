
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

#ifndef ETCOMPILERDIAGNOSTICCONSUMER_H
#define ETCOMPILERDIAGNOSTICCONSUMER_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/diagnostic.h>
#include <EmexToolchain/ETCompiler/ETCompilerOptions.h>

typedef struct __ETCompilerDiagnosticConsumer *ETCompilerDiagnosticConsumerRef;

EFTypeID ETCompilerDiagnosticConsumerGetTypeID(void);

ETCompilerDiagnosticConsumerRef ETCompilerDiagnosticConsumerCreate(EFAllocatorRef allocatorRef, ETCompilerDiagnosticOptions diagnosticOptions);

void ETCompilerDiagnosticConsumerSetDiagnosticOptions(ETCompilerDiagnosticConsumerRef consumerRef, ETCompilerDiagnosticOptions diagnosticOptions);
void ETCompilerDiagnosticConsumerReport(ETCompilerDiagnosticConsumerRef consumerRef, kDiagnosticSeverity severity, diagnostic_location_t *location, EFStringRef format, ...);
void ETCompilerDiagnosticConsumerEmit(ETCompilerDiagnosticConsumerRef consumerRef);

#endif /* ETCOMPILERDIAGNOSTICCONSUMER_H */
