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

#ifndef EMEX64ASM_DIAGNOSTIC_CONSUMER_H
#define EMEX64ASM_DIAGNOSTIC_CONSUMER_H

#include <stdio.h>
#include <stdlib.h>
#include <EmexToolchain/support/virtual/vfd.h>
#include <EmexToolchain/support/diagnostic/consumer.h>
#include <EmexToolchain/asm/options.h>

#define AT_TO_DLOC(at) &(diagnostic_location_t){ \
                .file_name = (at)->al->inv->file[(at)->al->file_idx]->path, \
                .line = (at)->al->str, \
                .ln = (at)->al->line_num, \
                .col = (at)->column_num, \
                .range = (diagnostic_location_text_range_t){ \
                    .start_col = (at)->column_num, \
                    .end_col = (at)->column_num + (at)->real_len } \
            }

typedef diagnostic_consumer_t assembler_diagnostic_consumer_t;

typedef struct assembler_diagnostic_consumer_context {
    assembler_diagnostic_options_t options;
    diagnostic_t **diagnostic;
    uint64_t diagnostic_cnt;
    vfd_t *d;
} assembler_diagnostic_consumer_context_t;

assembler_diagnostic_consumer_t *assembler_diagnostic_consumer_alloc(assembler_diagnostic_options_t options);
void assembler_diagnostic_consumer_dealloc(assembler_diagnostic_consumer_t *consumer);

void assembler_diagnostic_consumer_emit(assembler_diagnostic_consumer_t *consumer);

#endif /* EMEX64ASM_DIAGNOSTIC_CONSUMER_H */
