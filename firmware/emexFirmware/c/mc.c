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

#include <fw.h>

u64 mc_get_size(void)
{
    return mmio_read64(EMEX64_MC_REG_SIZE);
}

void mc_protect_ktrr(void)
{
    mmio_write64(EMEX64_MC_REG_KTRR_SIZE, 0);
    mmio_write64(EMEX64_MC_REG_KTRR_BASE, 0);
    mmio_write64(EMEX64_MC_REG_KTRR_SIZE, (u64)__data_start);
}

void mc_lock_ktrr(void)
{
    mmio_write64(EMEX64_MC_REG_KTRR_LOCKED, 1);
}
