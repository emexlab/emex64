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

#ifndef EMEX64LD_TYPE_H
#define EMEX64LD_TYPE_H

#include <EmexFoundation/EmexFoundation.h>

#define ELF_MAGIC_EMEX64    0x0E64

#define ELF_MAGIC_0 0x7f
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELF_CLASS64     2
#define ELF_DATA2LSB    1

#define EV_CURRENT  1

typedef enum: UInt8 {
    kELFTypeNone =  0,
    kELFTypeRel =   1,
    kELFTypeExec =  2,
} kELFType;

typedef enum: UInt8 {
    kELFSectionHeaderTypeNull =     0,
    kELFSectionHeaderTypeProgbits = 1,
    kELFSectionHeaderTypeSymtab =  2,
    kELFSectionHeaderTypeStrtab =  3,
    kELFSectionHeaderTypeRelative = 4,
    kELFSectionHeaderTypeNobits =   8,
} kELFSectionHeaderType;

typedef enum: UInt8 {
    kELFSectionFlagWrite =  (1 << 0),
    kELFSectionFlagAlloc =  (1 << 1),
    kELFSectionFlagExec =   (1 << 2),
} kELFSectionFlag;

typedef enum: UInt8 {
    kELFSymbolTableBindingLocal =   0,
    kELFSymbolTableBindingGlobal =  1,
    kELFSymbolTableBindingWeak =    2,
} kELFSymbolTableBinding;

typedef enum: UInt8 {
    kELFSymbolTableTypeNoType =     0,
    kELFSymbolTableTypeObject =     1,
    kELFSymbolTableTypeFunc =       2,
    kELFSymbolTableTypeSection =    3,
    kELFSymbolTableTypeFile =       4,
} kELFSymbolTableType;

typedef enum: UInt8 {
    kELFSymbolVisibilityDefault =   0,
    kELFSymbolVisibilityInternal =  1,
    kELFSymbolVisibilityHidden =    2,
    kELFSymbolVisibilityProtected = 3,
} kELFSymbolVisibility;

typedef enum: UInt16 {
    kELFSectionHeaderNumberUndefined =  0,
    kELFSectionHeaderNumberAbsolute =   0xFFF1,
} kELFSectionHeaderNumber;

typedef enum: UInt16 {
    kELFSectionHeaderIndexNull =        0,
    kELFSectionHeaderIndexText =        1,
    kELFSectionHeaderIndexData =        2,
    kELFSectionHeaderIndexBSS =         3,
    kELFSectionHeaderIndexRelaText =    4,
    kELFSectionHeaderIndexRelaData =    5,
    kELFSectionHeaderIndexSymtab =      6,
    kELFSectionHeaderIndexStrtab =      7,
    kELFSectionHeaderIndexShstrtab =    8,
    kELFSectionHeaderIndexCount =       9,
} kELFSectionHeaderIndex;

typedef enum: UInt8 {
    kEmitModeNone,
    kEmitModeFirmware,
    kEmitModeRelocatableObject,
} kEmitMode;

#define R_EMEX64_ABS64  1

#define EI_NIDENT   16

#define ELF32_R_SYM(i) ((i) >> 32)
#define ELF32_R_TYPE(i) ((i) & 0xFFFFFFFF)
#define ELF64_R_INFO(s,t) (((UInt64)(s) << 32) | (UInt32)(t))

#define ELF_SYM_INFO(bind, type) (((bind) << 4) | ((type) & 0xf))

extern const UInt8 ident[EI_NIDENT];

#endif /* EMEX64LD_TYPE_H */
