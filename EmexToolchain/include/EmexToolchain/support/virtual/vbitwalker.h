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

#ifndef EMEX64_VBITWALKER_H
#define EMEX64_VBITWALKER_H

#include <EmexToolchain/support/virtual/vfd.h>
#include <EmexToolchain/support/endian.h>

typedef struct {
    vfd_t *d;
    size_t byte_pos;
    uint8_t bit_idx;
    bw_endian_t endian;
} vbitwalker_t;

vbitwalker_t *vbitwalker_alloc(vfd_t *d, bw_endian_t endian);
void vbitwalker_dealloc(vbitwalker_t *fw);

void vbitwalker_reset(vbitwalker_t *fw);

int vbitwalker_write(vbitwalker_t *fw, uint64_t value, uint8_t num_bits);
uint64_t vbitwalker_read(vbitwalker_t *fw, uint8_t num_bits);
int vbitwalker_write_buf(vbitwalker_t *fw, const char *buf, size_t len);
int vbitwalker_read_buf(vbitwalker_t *fw, char *buf, size_t len);

void vbitwalker_seek(vbitwalker_t *fw, size_t byte_pos, uint8_t bit_idx);

void vbitwalker_skip(vbitwalker_t *fw, size_t num_bits);

size_t vbitwalker_bytes_used(const vbitwalker_t *fw);

void vbitwalker_align_byte(vbitwalker_t *fw);

void vbitwalker_sync(vbitwalker_t *fw);

#endif /* EMEX64_VBITWALKER_H */
