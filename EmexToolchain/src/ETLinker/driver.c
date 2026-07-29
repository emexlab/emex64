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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/version.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETLinker/linker.h>
#include <EmexToolchain/ETLinker/driver.h>
#include <EmexToolchain/ETLinker/emit.h>

linker_driver_t *linker_driver_alloc(SInt32 argc,
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

    driver->inputFiles = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, (EFIndex)argc);
    if(driver->inputFiles == NULL)
    {
        linker_diagnostic_consumer_dealloc(driver->consumer);
        free(driver);
        return NULL;
    }

    driver->linkerScriptFiles = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, (EFIndex)argc);
    if(driver->linkerScriptFiles == NULL)
    {
        EFRelease(driver->inputFiles);
        linker_diagnostic_consumer_dealloc(driver->consumer);
        free(driver);
        return NULL;
    }

    for(SInt32 i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Options:\n");
            fprintf(stderr, "  --help                 Shows this help menu.\n");
            fprintf(stderr, "  --version              Prints version.\n");
            fprintf(stderr, "  --omagic               Uses old magic (merges .text and .data into one read-write\n");
            fprintf(stderr, "                         block, note that this could be a security risk).\n");
            fprintf(stderr, "  --nmagic               Uses new magic (separates .text and .data).\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "  -o <output path>       Sets the output file path, is set to \"a.out\" when not passed.\n");
            fprintf(stderr, "  -e <entry name>        Sets the entry symbol, is set to \"_start\" when not passed.\n");
            fprintf(stderr, "  -T <script path>       Adds a linker script.\n");
            fprintf(stderr, "  -v                     Prints verbose linker log.\n");
            fprintf(stderr, "  -r                     Emits relocatable ELF object.\n");
            goto failure;
        }
        else if(strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "%s version %d.%d.%d (%s)\n", argv[0], EMEX64_VERSION_MAJOR, EMEX64_VERSION_MINOR, EMEX64_VERSION_PATCH, EMEX64_VERSION_STRING);
            goto failure;
        }
        else if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            EFReleaseTry(driver->output_file);
            EFAUTOREL EFStringRef pathStr = EFStringCreateWithCString(kEFAllocatorDefault, argv[++i], kEFStringEncodingUTF8);
            driver->output_file = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyOutData, pathStr);
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
            EFAUTOREL EFStringRef pathStr = EFStringCreateWithCString(kEFAllocatorDefault, argv[++i], kEFStringEncodingUTF8);
            EFAUTOREL EFFileRef scriptFile = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyInData, pathStr);
            if(!EFArrayAppendValue(driver->linkerScriptFiles, scriptFile))
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing script file '%s'", argv[i]);
                goto failure;
            }
        }
        else if (strncmp(argv[i], "-T", 2) == 0 && argv[i][2])
        {
            EFAUTOREL EFStringRef pathStr = EFStringCreateWithCString(kEFAllocatorDefault, argv[i] + 2, kEFStringEncodingUTF8);
            EFAUTOREL EFFileRef scriptFile = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyInData, pathStr);
            if(!EFArrayAppendValue(driver->linkerScriptFiles, scriptFile))
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing script file '%s'", argv[i]);
                goto failure;
            }
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
            EFAUTOREL EFStringRef pathStr = EFStringCreateWithCString(kEFAllocatorDefault, argv[i], kEFStringEncodingUTF8);
            EFAUTOREL EFFileRef inputFile = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyInData, pathStr);
            if(!EFArrayAppendValue(driver->inputFiles, inputFile))
            {
                diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown or non existing input file '%s'", argv[i]);
                goto failure;
            }
        }
        else
        {
            diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "unknown option '%s'", argv[i]);
            goto failure;
        }
    }

    if(EFArrayGetCount(driver->inputFiles) <= 0)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityError, NULL, "no input files");
        goto failure;
    }

    /* fallback to a.out if not passed */
    if(driver->output_file == NULL)
    {
        diagnostic_report(driver->consumer, kDiagnosticSeverityWarning, NULL, "no output binary specified, falling back to 'a.out'");
        driver->output_file = EFFileCreateWithPath(kEFAllocatorDefault, EFFilePolicyOutData, EFSTR("a.out"));
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
    EFRelease(driver->linkerScriptFiles);
    EFRelease(driver->inputFiles);
    EFReleaseTry(driver->output_file);
    linker_diagnostic_consumer_emit(driver->consumer);
    linker_diagnostic_consumer_dealloc(driver->consumer);
    free(driver);
}

Boolean linker_driver_drive_the_fucking_car(linker_driver_t *driver)
{
    linker_invocation_t *inv = linker_invocation_alloc(driver->options, driver->consumer);
    if(inv == NULL)
    {
        return false;
    }

    Boolean success = linker_link(inv, driver->inputFiles, driver->linkerScriptFiles, driver->output_file);
    if(!success)
    {
        linker_invocation_dealloc(inv);
        EFFileUnlink(driver->output_file);
    }

    linker_invocation_dealloc(inv);
    return success;
}
