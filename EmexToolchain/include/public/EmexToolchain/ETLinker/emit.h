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

#ifndef EMEX64LD_EMIT_H
#define EMEX64LD_EMIT_H

#include <EmexToolchain/Support/virtual/vbitwalker.h>
#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETLinker/type.h>
#include <EmexToolchain/ETLinker/options.h>
#include <EmexToolchain/ETLinker/diagnostic/consumer.h>

typedef struct linker_invocation linker_invocation_t;

Boolean linker_link(linker_options_t options, linker_diagnostic_consumer_t *diagnostic_consumer, emex_file_t **input_file, UInt64 input_file_cnt, emex_file_t **linker_script_file, UInt64 linker_script_file_cnt, emex_file_t *output);

#endif /* EMEX64LD_EMIT_H */
