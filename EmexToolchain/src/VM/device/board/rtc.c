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

#include <EmexToolchain/VM/device/board/rtc.h>
#include <time.h>

UInt64 emex64_rtc_read(E64CoreRef core,
                       void *device,
                       UInt64 offset,
                       int size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    switch(offset)
    {
        case RTC_REG_SECONDS:
            return t->tm_sec;
        case RTC_REG_MINUTES:
            return t->tm_min;
        case RTC_REG_HOURS:
            return t->tm_hour;
        case RTC_REG_DAY:
            return t->tm_mday;
        case RTC_REG_MONTH:
            return t->tm_mon + 1;
        case RTC_REG_YEAR:
            return (UInt64)((t->tm_year + 1900) % 100);
        case RTC_REG_WEEKDAY:
            return t->tm_wday;
        default:
            return 0;
    }
}
