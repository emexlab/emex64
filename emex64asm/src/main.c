/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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
#include <string.h>
#include <unistd.h>
#include <emex64asm/compiler.h>
#include <emex64asm/code.h>
#include <emex64asm/label.h>
#include <emex64asm/emit.h>
#include <emex64asm/section.h>
#include <emex64asm/macro.h>
#include <emex64asm/diag.h>

int main(int argc, char *argv[])
{
    int opt;
    const char *output_path = NULL;
    int file_count = 0;
    char **files = calloc(argc, sizeof(char *));

    /* invocation settings */
    const char *start_entry_name = "_start";
    bool page_align = true;
    bool warning_deprecated = true;

    /* parse arguments */
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            output_path = argv[++i];
        }
        else if(strncmp(argv[i], "-f", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-f'\n");
            }

            if(strcmp(flag, "page_align") == 0)
            {
                page_align = true;
            }
            else if(strcmp(flag, "no_page_align") == 0)
            {
                page_align = false;
            }
            else
            {
                diag_error(NULL, "unknown feature flag '%s'\n", flag);
            }
        }
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-W'\n");
            }

            if(strcmp(flag, "error") == 0)
            {
                warning_error = true;
            }
            else if(strcmp(flag, "no_error") == 0)
            {
                warning_error = false;
            }
            else if(strcmp(flag, "deprecated") == 0)
            {
                warning_deprecated = true;
            }
            else if(strcmp(flag, "no_deprecated") == 0)
            {
                warning_deprecated = false;
            }
            else
            {
                diag_error(NULL, "unknown warning flag '%s'\n", flag);
            }
        }
        else if(strncmp(argv[i], "-e", 2) == 0)
        {
            const char *flag;
            if(argv[i][2] != '\0')
            {
                flag = argv[i] + 2;
            }
            else if(i + 1 < argc)
            {
                flag = argv[++i];
            }
            else
            {
                diag_error(NULL, "missing argument to '-e'\n");
            }

            start_entry_name = flag;
        }
        else if(argv[i][0] != '-')
        {
            files[file_count++] = strdup(argv[i]);
        }
        else
        {
            diag_error(NULL, "unknown option '%s'\n", argv[i]);
        }
    }

    /* checking for output path */
    if(!output_path)
    {
        diag_warn(NULL, "no output binary specified, falling back to a.out\n");
        output_path = "a.out";
    }

    /* allocating compiler invocation */
    compiler_invocation_t *ci = compiler_invocation_alloc(output_path);

    if(ci == NULL)
    {
        diag_error(NULL, "something went terribly wrong\n");
    }

    ci->page_align = page_align;
    ci->start_entry_name = start_entry_name;
    ci->warning_error = warning_error;
    ci->warning_deprecated = warning_deprecated;

    /* remaining arguments are input files */
    if(file_count <= 0)
    {
        diag_error(NULL, "no input files provided\n");
    }

    /* generating tokens,labels,sections out of the code */
    code_tokengen(ci, (const char **)files, file_count);

    /* doing parsing acrobatic */
    code_token_label(ci);
    code_token_section(ci);
    code_token_macro(ci);

    /* finally compiling it to machine code */
    la64_compiler_emit_all(ci);

    /* insert entry */
    code_token_label_insert_start(ci);

    /* its oneshot */
    /* compiler_invocation_dealloc(ci); */

    /* cleanup */
    for(int i = 0; i < file_count; i++)
    {
        free(files[i]);
    }
    free(files);

    return 0;
}
