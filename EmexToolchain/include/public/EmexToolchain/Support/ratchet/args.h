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

#ifndef EMEX_RATCHET_ARGS_H
#define EMEX_RATCHET_ARGS_H

#include <EmexFoundation/EmexFoundation.h>

typedef struct ratchet_args {
    Boolean failed;     /* the latch */
    EFMutableArrayRef array;
} ratchet_args_t;

Boolean ratchet_args_init(ratchet_args_t *ra);
void ratchet_args_deinit(ratchet_args_t *ra);
void ratchet_args_append(ratchet_args_t *ra, const char *arg);

#endif /* EMEX_RATCHET_ARGS_H */
