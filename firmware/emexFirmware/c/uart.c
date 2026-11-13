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
#include <uart.h>

#define EMEX64_UART_REG_DATA    (EMEX64_UART_BASE + 0x00)
#define EMEX64_UART_REG_STATUS  (EMEX64_UART_BASE + 0x08)
#define EMEX64_UART_REG_CONTROL (EMEX64_UART_BASE + 0x10)

#define EMEX64_UART_STATUS_RX_READY 0x01
#define EMEX64_UART_STATUS_TX_EMPTY 0x02

void uart_putc(const char c)
{
    while((mmio_read64(EMEX64_UART_REG_STATUS) & EMEX64_UART_STATUS_TX_EMPTY) == 0) {};
    mmio_write64(EMEX64_UART_REG_DATA, (u64)c);
}

char uart_getc(void)
{
    while((mmio_read64(EMEX64_UART_REG_STATUS) & EMEX64_UART_STATUS_RX_READY) == 0) {};
    return (char)mmio_read64(EMEX64_UART_REG_DATA);
}
