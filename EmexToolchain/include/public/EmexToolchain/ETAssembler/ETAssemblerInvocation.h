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

#ifndef ETASSEMBLERINVOCATION_H
#define ETASSEMBLERINVOCATION_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/hashmap/hashmap.h>
#include <EmexToolchain/ETAssembler/diagnostic/ETAssemblerDiagnosticConsumer.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/ETAssembler/ETAssemblerOptions.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/ETAssembler/__ETAssemblerInvocation.h>
#endif /* ET_PRIVATE */

typedef struct {
    char *match;
    char *value;
} assembler_macro_definition_t;

typedef struct __ETAssemblerInvocation *ETAssemblerInvocationRef;

EFTypeID ETAssemblerInvocationGetTypeID(void);

ETAssemblerInvocationRef ETAssemblerInvocationCreate(EFAllocatorRef allocatorRef, ETAssemblerDiagnosticConsumerRef diagnosticConsumer);

Boolean ETAssemblerInvocationAddMacroDefinition(ETAssemblerInvocationRef invocationRef, assembler_macro_definition_t *macro);
Boolean ETAssemblerInvocationAddIncludeSearchPath(ETAssemblerInvocationRef invocationRef, EFStringRef includeSearchPath);

Boolean ETAssemblerInvocationEmit(ETAssemblerInvocationRef invocationRef);

EFFileRef ETAssemblerInvocationGetInputFile(ETAssemblerInvocationRef invocationRef);
EFFileRef ETAssemblerInvocationGetOutputFile(ETAssemblerInvocationRef invocationRef);

Boolean ETAssemblerInvocationSetInputFile(ETAssemblerInvocationRef invocationRef, EFFileRef inputFile);
Boolean ETAssemblerInvocationSetOutputFile(ETAssemblerInvocationRef invocationRef, EFFileRef outputFile);

Boolean ETAssemblerInvocationHasErrorOccured(ETAssemblerInvocationRef invocationRef);
Boolean ETAssemblerInvocationHasRan(ETAssemblerInvocationRef invocationRef);

#endif /* ETASSEMBLERINVOCATION_H */
