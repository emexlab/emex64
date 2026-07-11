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

#ifndef EMEX64ASM_DRIVER_H
#define EMEX64ASM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <EmexToolchain/support/file.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/linker/type.h>

typedef enum: UInt8 {
    kAssemblerJobTypeAssembler,
    kAssemblerJobTypeDriver,
    kAssemblerJobTypeLinker
} kAssemblerJobType;

typedef struct assembler_job {
    kAssemblerJobType type;
    const char *command;        /* borrowed */
    char **argv;
    int argc;
    struct assembler_job *prev;
    struct assembler_job *next;
} assembler_job_t;

typedef struct {
    ETAssemblerDriverOptions options;
    ETAssemblerDiagnosticOptions diagnosticOptions;
    assembler_diagnostic_consumer_t *consumer;  /* owned */
    kEmitMode emit_mode;

    const char *output_path;    /* borrowed */

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

    assembler_job_t *job;
} assembler_driver_t;

assembler_job_t *assembler_job_alloc(assembler_job_t *prev, kAssemblerJobType type, const char *command, int argc, const char **argv);
void assembler_job_dealloc(assembler_job_t *job);

assembler_driver_t *assembler_driver_alloc(int argc, const char **argv);
void assembler_driver_dealloc(assembler_driver_t *driver);

Boolean assembler_driver_drive_the_fucking_car(assembler_driver_t *driver);

#endif /* EMEX64ASM_DRIVER_H */
