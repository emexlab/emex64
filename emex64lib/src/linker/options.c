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

#include <stdlib.h>
#include <string.h>

#include <emex64lib/support/diag.h>

#include <emex64lib/linker/options.h>

linker_options_t *linker_options_alloc(void)
{
    linker_options_t *options = malloc(sizeof(linker_options_t));
    if(options == NULL)
    {
        return NULL;
    }

    options->output_path = NULL;
    options->entry_name = NULL;
    options->verbose = false;
    options->emit_mode = kEmitModeFirmware;

    return options;
}

void linker_options_dealloc(linker_options_t *options)
{
    free(options->output_path);
    free(options->entry_name);
}

const char *linker_options_get_output_path(linker_options_t *options)
{
    if(options->output_path == NULL)
    {
        diag_warn(NULL, "no output binary specified, falling back to a.out\n");
        options->output_path = strdup("a.out");
    }
    return options->output_path;
}

const char *linker_options_get_entry_name(linker_options_t *options)
{
    if(options->entry_name == NULL)
    {
        options->entry_name = strdup("_start");
    }
    return options->entry_name;
}
