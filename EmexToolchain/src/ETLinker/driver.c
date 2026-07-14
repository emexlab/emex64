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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <EmexToolchain/Support/version.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETLinker/linker.h>
#include <EmexToolchain/ETLinker/driver.h>
#include <EmexToolchain/ETLinker/emit.h>

linker_driver_t *linker_driver_alloc(int argc,
                                     const char **argv)
{
    /* slightly different from the assembler driver lol */
    linker_driver_t *driver = calloc(1, sizeof(linker_driver_t));
    if(driver == NULL)
    {
        return NULL;
    }

    driver->consumer = linker_diagnostic_consumer_alloc();
    if(driver->consumer == NULL)
    {
        free(driver);
        return NULL;
    }

    driver->options = linker_options_default;

    driver->output_file = NULL;

    driver->input_file = calloc(argc, sizeof(emex_file_t));
    driver->input_file_cnt = 0;

    driver->linker_script_file = calloc(argc, sizeof(emex_file_t));
    driver->linker_script_file_cnt = 0;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  --help                 Shows this help menu.\n");
            fprintf(stderr, "  --version              Prints version.\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -o <output path>       Sets the output file path, is set to \"a.out\" when not passed.\n");
            fprintf(stderr, "  -e <entry name>        Sets the entry symbol, is set to \"_start\" when not passed.\n");
            fprintf(stderr, "  -T <script path>       Adds a linker script.\n");
            fprintf(stderr, "  -v                     Prints verbose linker log.\n");
            fprintf(stderr, "  -r                     Emits relocatable ELF object.\n");
            fprintf(stderr, "  -omagic                Uses old magic (merges .text and .data into one read-write\n");
            fprintf(stderr, "                         block, note that this could be a security risk).\n");
            fprintf(stderr, "  -nmagic                Uses new magic (separates .text and .data).\n");
            goto failure;
        }
        else if(strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "%s version %d.%d.%d (%s)\n", argv[0], EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
            goto failure;
        }
        else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            emex_file_dealloc(driver->output_file);
            driver->output_file = emex_file_alloc(argv[++i], out_data_file_policy);
            if(driver->output_file == NULL)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "don't have permission to open file at '%s'", argv[i]);
                goto failure;
            }
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
        {
            driver->options.entry_name = argv[++i];
        }
        else if((strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--script") == 0) && i + 1 < argc)
        {
            emex_file_t *script_file = emex_file_alloc(argv[++i], in_data_file_policy);
            if(script_file == NULL)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing script file '%s'", argv[i]);
                goto failure;
            }
            driver->linker_script_file[driver->linker_script_file_cnt++] = script_file;
        }
        else if (strncmp(argv[i], "-T", 2) == 0 && argv[i][2])
        {
            emex_file_t *script_file = emex_file_alloc(argv[i] + 2, in_data_file_policy);
            if(script_file == NULL)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing script file '%s'", argv[i] + 2);
                goto failure;
            }
            driver->linker_script_file[driver->linker_script_file_cnt++] = script_file;
        }
        else if(strcmp(argv[i], "-v") == 0)
        {
            driver->options.verbose = true;
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            driver->options.emit_mode = kEmitModeRelocatableObject;
        }
        else if(strcmp(argv[i], "--nmagic") == 0)
        {
            driver->options.use_old_magic = false;
        }
        else if(strcmp(argv[i], "--omagic") == 0)
        {
            driver->options.use_old_magic = true;
        }
        else if (argv[i][0] != '-')
        {
            emex_file_t *input_file = emex_file_alloc(argv[i], in_data_file_policy);
            if(input_file == NULL)
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing input file '%s'", argv[i]);
                goto failure;
            }
            driver->input_file[driver->input_file_cnt++] = input_file;
        }
        else
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown option '%s'", argv[i]);
            goto failure;
        }
    }

    if(driver->input_file_cnt <= 0)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "no input files");
        goto failure;
    }

    /* fallback to a.out if not passed */
    if(driver->output_file == NULL)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityWarning, NULL, "no output binary specified, falling back to 'a.out'");
        driver->output_file = emex_file_alloc("a.out", out_data_file_policy);
        if(driver->output_file == NULL)
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "don't have permission to open file at 'a.out'");
        }
    }

    return driver;

failure:
    linker_driver_dealloc(driver);
    return NULL;
}

void linker_driver_dealloc(linker_driver_t *driver)
{
    for(UInt64 i = 0; i < driver->input_file_cnt; i++)
    {
        emex_file_dealloc(driver->input_file[i]);
    }
    free(driver->input_file);

    for(UInt64 i = 0; i < driver->linker_script_file_cnt; i++)
    {
        emex_file_dealloc(driver->linker_script_file[i]);
    }
    free(driver->linker_script_file);
    emex_file_dealloc(driver->output_file);
    linker_diagnostic_consumer_emit(driver->consumer);
    linker_diagnostic_consumer_dealloc(driver->consumer);
    free(driver);
}

Boolean linker_driver_drive_the_fucking_car(linker_driver_t *driver)
{
    Boolean success = linker_link(driver->options, driver->consumer, driver->input_file, driver->input_file_cnt, driver->linker_script_file, driver->linker_script_file_cnt, driver->output_file);
    if(!success)
    {
        emex_file_unlink(driver->output_file);
    }
    return success;
}
