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

#ifndef EMEX64LD_HEADER_H
#define EMEX64LD_HEADER_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETLinker/type.h>

typedef struct {
    UInt8 e_ident[EI_NIDENT];
    UInt16 e_type;
    UInt16 e_machine;
    UInt32 e_version;
    UInt64 e_entry;
    UInt64 e_phoff;
    UInt64 e_shoff;
    UInt32 e_flags;
    UInt16 e_ehsize;
    UInt16 e_phentsize;
    UInt16 e_phnum;
    UInt16 e_shentsize;
    UInt16 e_shnum;
    UInt16 e_shstrndx;
} __attribute__((packed)) ELF64_Ehdr;

typedef struct {
    UInt32 sh_name;
    UInt32 sh_type;
    UInt64 sh_flags;
    UInt64 sh_addr;
    UInt64 sh_offset;
    UInt64 sh_size;
    UInt32 sh_link;
    UInt32 sh_info;
    UInt64 sh_addralign;
    UInt64 sh_entsize;
} __attribute__((packed)) ELF64_Shdr;

#define BOOT_HEADER_SIZE    10

#endif /* EMEX64LD_HEADER_H */
