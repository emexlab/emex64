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

#ifndef EMEX64LD_DRIVER_H
#define EMEX64LD_DRIVER_H

#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETLinker/options.h>
#include <EmexToolchain/ETLinker/diagnostic/consumer.h>

typedef struct {
    linker_options_t options;
    linker_diagnostic_consumer_t *consumer; /* owned */

    emex_file_t *output_file;

    emex_file_t **input_file;               /* borrowed */
    UInt64 input_file_cnt;

    emex_file_t **linker_script_file;       /* borrowed */
    UInt64 linker_script_file_cnt;
} linker_driver_t;

linker_driver_t *linker_driver_alloc(int argc, const char **argv);
void linker_driver_dealloc(linker_driver_t *driver);

Boolean linker_driver_drive_the_fucking_car(linker_driver_t *driver);

#endif /* EMEX64LD_DRIVER_H */
