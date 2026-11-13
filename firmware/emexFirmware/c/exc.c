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
#include <interrupt.h>

static const char *exception_str;
static const char *exception_names[] = {
    "unknown",
    "bad access",
    "permission denied",
    "bad instruction",
    "bad arithmetic",
    "page fault",
    "ktrr violation",
    "svc",
    "pointer authentication",
};

static const char *exception_str_for_code(u64 code)
{
    if(exception_str != NULL)
    {
        return exception_str;
    }
    if(code >= sizeof(exception_names) / sizeof(exception_names[0]))
    {
        return exception_names[0];
    }
    return exception_names[code];
}

static void exception_putreg(const char *name,
                             u64 value)
{
    fw_printf("%s: 0x%lx (%lu)\n", name, value, value);
}

void exception_handler(struct fw_frame *frame)
{
    u64 code = fw_cr_read(FW_CR_CREXC);

    fw_printf("\n-- panic --\nexception code: 0x%lx\n", code);
    fw_printf("reason: %s\n\n", exception_str_for_code(code));

    /* register dump by reading the interrupt stack frame */
    fw_printf("-- register dump --\n");
    exception_putreg("pc", frame->pc);
    exception_putreg("sp", frame->sp);
    exception_putreg("fp", frame->fp);
    exception_putreg("cf", frame->cf);
    exception_putreg("fpc", frame->fpc);
    exception_putreg("r0", frame->r0);
    exception_putreg("r1", frame->r1);
    exception_putreg("r2", frame->r2);
    exception_putreg("r3", frame->r3);
    exception_putreg("r4", frame->r4);
    exception_putreg("r5", frame->r5);
    exception_putreg("r6", frame->r6);
    exception_putreg("r7", frame->r7);
    exception_putreg("r8", frame->r8);
    exception_putreg("r9", frame->r9);
    exception_putreg("rr", frame->rr);

    interrupt_unregister_all();

    frame->pc = (u64)fw_hang;
    fw_cr_write(FW_CR_CREXC, 0);
}

void panic(const char *why)
{
    exception_str = why;
    u64 zero = 0;
    __asm__ __volatile__("div %0, %0" : "+r"(zero) : : "memory");
    fw_hang();
}

void fw_hang(void)
{
    fw_printf("[hang] the computer has likely died, you shall turn it off\n");
    for(;;)
    {
        fw_hlt();
    }
}
