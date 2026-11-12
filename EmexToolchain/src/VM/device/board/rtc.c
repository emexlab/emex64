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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/rtc.h>

UInt64 emex64_rtc_read(E64CoreRef core,
                       void *device,
                       UInt64 offset,
                       EFSize size)
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

emex64_nvram_t *emex64_nvram_alloc(const char *path)
{
    emex64_nvram_t *nvram = calloc(1, sizeof(emex64_nvram_t));
    if(nvram == NULL)
    {
        return NULL;
    }
    nvram->fd = -1;

    if(path != NULL)
    {
        nvram->fd = open(path, O_RDWR | O_CREAT, 0644);
        if(nvram->fd < 0)
        {
            fprintf(stderr, "emex64vm: cannot open the NVRAM file %s, it is kept in memory only\n", path);
        }
        else
        {
            /* a new or short file reads as zeros */
            if(pread(nvram->fd, nvram->data, EMEX64_NVRAM_BYTES, 0) < 0 ||
               ftruncate(nvram->fd, EMEX64_NVRAM_BYTES) != 0)
            {
                fprintf(stderr, "emex64vm: cannot use the NVRAM file %s, it is kept in memory only\n", path);
                close(nvram->fd);
                nvram->fd = -1;
            }
        }
    }

    return nvram;
}

void emex64_nvram_dealloc(emex64_nvram_t *nvram)
{
    if(nvram->fd >= 0)
    {
        close(nvram->fd);
    }
    free(nvram);
}

/* little endian in the file too, whatever the host is (the MMIO bus lock is held) */
UInt64 emex64_nvram_read(E64CoreRef core,
                         void *device,
                         UInt64 offset,
                         EFSize size)
{
    emex64_nvram_t *nvram = device;

    if(offset == NVRAM_REG_SIZE)
    {
        return EMEX64_NVRAM_BYTES;
    }
    if(offset + 8 > EMEX64_NVRAM_BYTES)
    {
        return 0;
    }

    UInt64 value = 0;
    for(int i = 0; i < 8; i++)
    {
        value |= (UInt64)nvram->data[offset + i] << (8 * i);
    }
    return value;
}

void emex64_nvram_write(E64CoreRef core,
                        void *device,
                        UInt64 offset,
                        UInt64 value,
                        EFSize size)
{
    emex64_nvram_t *nvram = device;

    if(offset + 8 > EMEX64_NVRAM_BYTES)
    {
        return;
    }

    for(int i = 0; i < 8; i++)
    {
        nvram->data[offset + i] = (UInt8)(value >> (8 * i));
    }
    if(nvram->fd >= 0 && pwrite(nvram->fd, &nvram->data[offset], 8, (off_t)offset) != 8)
    {
        fprintf(stderr, "emex64vm: writing the NVRAM file failed\n");
    }
}
