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

#ifndef EMEX64LD_DRIVER_H
#define EMEX64LD_DRIVER_H

#include <stdint.h>

#include <emex64lib/support/file.h>

#include <emex64lib/linker/options.h>

typedef struct {
    linker_options_t *options;

    emex_file_t **input_file;           /* borrowed */
    uint64_t input_file_cnt;

    emex_file_t **linker_script_file;   /* borrowed */
    uint64_t linker_script_file_cnt;
} linker_driver_t;

linker_driver_t *linker_driver_alloc(const char **argv, int argc);
void linker_driver_dealloc(linker_driver_t *driver);

bool linker_driver_drive_the_fucking_car(linker_driver_t *driver);

#endif /* EMEX64LD_DRIVER_H */
