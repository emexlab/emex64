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
 
OUTPUT_FORMAT("elf64-emex64")
ENTRY(_start)

SECTIONS
{
    . = 0;
    __image_start = .;
    .boot : {
        /*
         * the smallest header you have seen in your life:
         *
         * b _start
         */
        BYTE(0x26) BYTE(0x07) QUAD(_start)
    }
    __text_start = .;
    .text : { *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    . = ALIGN(0x2000);
    __data_start = .;
    .data : { *(.data .data.*) }
    .bss : {
        __bss_start = .;
        *(.bss .bss.*) *(COMMON)
        __bss_end = .;
    }
    __emex64_exec_img_end = .;

    /DISCARD/ : { *(.comment) *(.note.GNU-stack) *(.llvm_addrsig) }
}
