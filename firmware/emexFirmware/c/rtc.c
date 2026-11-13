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

#define EMEX64_RTC_REG_SECONDS  (EMEX64_RTC_BASE + 0x00)
#define EMEX64_RTC_REG_MINUTES  (EMEX64_RTC_BASE + 0x08)
#define EMEX64_RTC_REG_HOURS    (EMEX64_RTC_BASE + 0x10)
#define EMEX64_RTC_REG_DAY      (EMEX64_RTC_BASE + 0x18)
#define EMEX64_RTC_REG_MONTH    (EMEX64_RTC_BASE + 0x20)
#define EMEX64_RTC_REG_YEAR     (EMEX64_RTC_BASE + 0x28)
#define EMEX64_RTC_REG_WEEKDAY  (EMEX64_RTC_BASE + 0x30)

void print_time(void)
{
    fw_printf("[fw] date: %02lu.%02lu.%02lu %02lu:%02lu:%02lu\n",
              mmio_read64(EMEX64_RTC_REG_DAY),
              mmio_read64(EMEX64_RTC_REG_MONTH),
              mmio_read64(EMEX64_RTC_REG_YEAR),
              mmio_read64(EMEX64_RTC_REG_HOURS),
              mmio_read64(EMEX64_RTC_REG_MINUTES),
              mmio_read64(EMEX64_RTC_REG_SECONDS));
}
