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

/*
typedef struct __ETAssemblerDriver *ETAssemblerDriverRef;

ETAssemblerDriverRef ETAssemblerDriverCreateWithCArguments(EFAllocatorRef allocatorRef, int argc, const char **argv);
Boolean ETAssemblerDriverRun(ETAssemblerDriverRef driverRef);

EFArrayRef ETAssemblerGetJobs(ETAssemblerDriverRef driverRef);
EFArrayRef ETAssemblerCopyJobs(EFAllocatorRef allocatorRef, ETAssemblerDriverRef driverRef);

*/

typedef struct {
    ETAssemblerDriverOptions driverOptions;
    ETAssemblerDiagnosticOptions diagnosticOptions;
    ETAssemblerDiagnosticConsumerRef diagnosticConsumer;
    EFStringRef outputPath;
    EFArrayRef jobs;
    kEmitMode emit_mode;

    int input_file_count;
    emex_file_t **input_file;

    size_t inc_dir_cnt;
    char **inc_dirs;

    size_t tmp_path_cnt;
    char **tmp_paths;

    UInt64 macro_cnt;
    assembler_macro_definition_t *macro;

    int linker_flags_cnt;
    char **linker_flags;
} assembler_driver_t;

assembler_driver_t *assembler_driver_alloc(int argc, const char **argv);
void assembler_driver_dealloc(assembler_driver_t *driver);

Boolean assembler_driver_drive_the_fucking_car(assembler_driver_t *driver);

#endif /* ETASSEMBLERDRIVER_H */
