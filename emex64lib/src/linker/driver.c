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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <emex64lib/support/version.h>
#include <emex64lib/support/diagnostic/legacy.h>

#include <emex64lib/linker/linker.h>
#include <emex64lib/linker/driver.h>
#include <emex64lib/linker/emit.h>

linker_driver_t *linker_driver_alloc(int argc,
                                     const char **argv)
{
    /* slightly different from the assembler driver lol */
    linker_driver_t *driver = calloc(1, sizeof(linker_driver_t));
    if(driver == NULL)
    {
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
            fprintf(stderr, "  -r                     Emits relocatable object.\n");
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
            driver->output_file = emex_file_alloc(argv[++i], object_file_out_policy);
        }
        else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc)
        {
            driver->options.entry_name = argv[++i];
        }
        else if((strcmp(argv[i], "-T") == 0 || strcmp(argv[i], "--script") == 0) && i + 1 < argc)
        {
            emex_file_t *script_file = emex_file_alloc(argv[++i], linker_script_file_policy);
            if(script_file == NULL)
            {
                diag_error(NULL, "unknown or non existing script file '%s'\n", argv[i]);
                goto failure;
            }
            driver->linker_script_file[driver->linker_script_file_cnt++] = script_file;
        }
        else if (strncmp(argv[i], "-T", 2) == 0 && argv[i][2])
        {
            emex_file_t *script_file = emex_file_alloc(argv[i] + 2, linker_script_file_policy);
            if(script_file == NULL)
            {
                diag_error(NULL, "unknown or non existing script file '%s'\n", argv[i] + 2);
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
            diag_error(NULL, "relocatable object emission is not supported yet\n");
            goto failure;
        }
        else if (argv[i][0] != '-')
        {
            emex_file_t *input_file = emex_file_alloc(argv[i], object_file_load_policy);
            if(input_file == NULL)
            {
                diag_error(NULL, "unknown or non existing input file '%s'\n", argv[i]);
                goto failure;
            }
            driver->input_file[driver->input_file_cnt++] = input_file;
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
            goto failure;
        }
    }

    if(driver->input_file_cnt <= 0)
    {
        diag_error(NULL, "no input files\n");
        goto failure;
    }

    if(driver->output_file == NULL)
    {
        diag_warn(NULL, "no output binary specified, falling back to 'a.out'\n");
        driver->output_file = emex_file_alloc("a.out", object_file_out_policy);
    }

    return driver;

failure:
    for(uint64_t i = 0; i < driver->input_file_cnt; i++)
    {
        emex_file_dealloc(driver->input_file[i]);
    }
    free(driver->input_file);

    for(uint64_t i = 0; i < driver->linker_script_file_cnt; i++)
    {
        emex_file_dealloc(driver->linker_script_file[i]);
    }
    free(driver->linker_script_file);

    free(driver);
    return NULL;
}

void linker_driver_dealloc(linker_driver_t *driver)
{
    for(uint64_t i = 0; i < driver->input_file_cnt; i++)
    {
        emex_file_dealloc(driver->input_file[i]);
    }
    free(driver->input_file);

    for(uint64_t i = 0; i < driver->linker_script_file_cnt; i++)
    {
        emex_file_dealloc(driver->linker_script_file[i]);
    }
    free(driver->linker_script_file);
    emex_file_dealloc(driver->output_file);
    free(driver);
}

bool linker_driver_drive_the_fucking_car(linker_driver_t *driver)
{
    bool success = linker_link(driver->options, driver->input_file, driver->input_file_cnt, driver->linker_script_file, driver->linker_script_file_cnt, driver->output_file);
    if(!success)
    {
        emex_file_unlink(driver->output_file);
    }
    return success;
}
