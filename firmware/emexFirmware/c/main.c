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
#include <display.h>
#include <interrupt.h>

#define EMEXBOOT_COPYRIGHT_NOTICES_COUNT 2

const char *copyright_notices[EMEXBOOT_COPYRIGHT_NOTICES_COUNT] = {
    "LightweightArchitecture(c) 2024 - 2026",
    "emexLabs(c) 2026",
};
const int copyright_notices_count = EMEXBOOT_COPYRIGHT_NOTICES_COUNT;

void print_time(void);
u64 mc_get_size(void);
void mc_protect_ktrr(void);
void mc_lock_ktrr(void);
void fw_boot_main(void);

static u64 fw_memory_available(void)
{
    u64 fw_end = ((u64)__bss_end + EMEX64_PAGE_SIZE - 1) & ~(EMEX64_PAGE_SIZE - 1);
    u64 end = fw_ram_end();
    return end > fw_end ? end - fw_end : 0;
}

int main(void)
{
    /* finally C, how long I waited for this moment */
    put_init();
    interrupt_init();
    interrupt_register_handler(EMEX64_IC_IRQ_EXCEPTION, exception_handler);

    /* printing information about the ISA */
    fw_printf("[fw] isa: LA64-v%lu-ISA\n", (fw_cr_read(FW_CR_CRISA) >> 16) & 0xFFFF);

    /* printing information about memory */
    u64 total = mc_get_size() & ~(EMEX64_PAGE_SIZE - 1);
    fw_printf("[fw] memory installed: ");
    put_mem(total, total);
    fw_putc('\n');
    fw_printf("[fw] memory available: ");
    put_mem(fw_memory_available(), total);
    fw_putc('\n');
    print_time();

    /* printing information about the cores using the long awated APIC */
    fw_printf("[fw] cores: %lu\n", (mmio_read64(EMEX64_APIC_REG_VERSION) >> 8) & 0xFF);

    /* printing information about the display */
    fw_printf("[fw] display resolution: ");
    if(gui_height != 0)
    {
        fw_printf("%u x %u\n", gui_width, gui_height);
    }
    else
    {
        fw_printf("not installed\n");
    }

    mc_protect_ktrr();
    fw_printf("[fw] KTRR protects the firmware text\n");

    /* really hoping it finds some shit like Linux and boots into it, imagine this works */
    fw_boot_main();

    /* the fallback, mhhhh :< */
    mc_lock_ktrr();
    fw_printf("[fw] nothing booted, locked KTRR over the firmware\n");

    for(;;)
    {
        fw_hlt();
    }
}
