/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EMEX64ASM_DRIVER_H
#define EMEX64ASM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <emex64lib/support/file.h>

#include <emex64lib/asm/invocation.h>

#include <emex64lib/linker/type.h>

typedef enum: uint8_t {
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
    bool assemble_only;
    bool verbose;
    bool in_process;
} assembler_driver_options_t;

typedef struct {
    assembler_driver_options_t options;
    assembler_options_t invocation_options;
    kEmitMode emit_mode;

    const char *output_path;    /* borrowed */

    int input_path_count;
    char **input_path;
    kEmexFileType *input_path_type;

    size_t inc_dir_cnt;
    char **inc_dirs;

    size_t tmp_path_cnt;
    char **tmp_paths;

    uint64_t macro_cnt;
    assembler_macro_definition_t *macro;

    int linker_flags_cnt;
    char **linker_flags;

    assembler_job_t *job;
} assembler_driver_t;

assembler_job_t *assembler_job_alloc(assembler_job_t *prev, kAssemblerJobType type, const char *command, int argc, const char **argv);
void assembler_job_dealloc(assembler_job_t *job);

assembler_driver_t *assembler_driver_alloc(int argc, const char **argv);
void assembler_driver_dealloc(assembler_driver_t *driver);

bool assembler_driver_drive_the_fucking_car(assembler_driver_t *driver);

#endif /* EMEX64ASM_DRIVER_H */
