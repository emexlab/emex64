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

#ifndef __ETASSEMBLERINVOCATION_H
#define __ETASSEMBLERINVOCATION_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/hashmap/hashmap.h>
#include <EmexToolchain/ETAssembler/ETAssemblerDiagnosticConsumer.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/ETAssembler/ETAssemblerOptions.h>

typedef struct __ETAssemblerInvocation {
    EFObject header;

    ETAssemblerDiagnosticConsumerRef diagnosticConsumer;

    EFFileRef inputFile;
    EFFileRef outputFile;

    EFMutableArrayRef files;
    EFMutableArrayRef definitions;
    EFMutableArrayRef includeSearchPaths;

    assembler_line_t **line;
    UInt64 line_cnt;

    char *label_scope;
    hashmap_t *label_hashmap;

    reloc_table_entry_t *rtbe;
    EFBitWalkerRef out_vbitwalker;

    UInt64 data_section_start;
    UInt64 data_section_end;
    UInt64 bss_section_start;
    UInt64 bss_section_size;

    Boolean hasRan;
    Boolean hasErrorOccured;
} *__ETAssemblerInvocation;

#endif /* __ETASSEMBLERINVOCATION_H */
