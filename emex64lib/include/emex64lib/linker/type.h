/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EMEX64LD_TYPE_H
#define EMEX64LD_TYPE_H

#include <stdint.h>
#include <stdbool.h>

#define ELF_MAGIC_EMEX64    0x0E64

#define ELF_MAGIC_0 0x7f
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELF_CLASS64     2
#define ELF_DATA2LSB    1

#define EV_CURRENT  1

typedef enum: uint8_t {
    kELFTypeNone =  0,
    kELFTypeRel =   1,
    kELFTypeExec =  2,
} kELFType;

typedef enum: uint8_t {
    kELFSectionHeaderTypeNull =     0,
    kELFSectionHeaderTypeProgbits = 1,
    kELFSectionHeaderTypeSymtab =  2,
    kELFSectionHeaderTypeStrtab =  3,
    kELFSectionHeaderTypeRelative = 4,
    kELFSectionHeaderTypeNobits =   8,
} kELFSectionHeaderType;

typedef enum: uint8_t {
    kELFSectionFlagWrite =  (1 << 0),
    kELFSectionFlagAlloc =  (1 << 1),
    kELFSectionFlagExec =   (1 << 2),
} kELFSectionFlag;

typedef enum: uint8_t {
    kELFSymbolTableBindingLocal =   0,
    kELFSymbolTableBindingGlobal =  1,
    kELFSymbolTableBindingWeak =    2,
} kELFSymbolTableBinding;

typedef enum: uint8_t {
    kELFSymbolTableTypeNoType =     0,
    kELFSymbolTableTypeObject =     1,
    kELFSymbolTableTypeFunc =       2,
    kELFSymbolTableTypeSection =    3,
    kELFSymbolTableTypeFile =       4,
} kELFSymbolTableType;

typedef enum: uint8_t {
    kELFSymbolVisibilityDefault =   0,
    kELFSymbolVisibilityInternal =  1,
    kELFSymbolVisibilityHidden =    2,
    kELFSymbolVisibilityProtected = 3,
} kELFSymbolVisibility;

typedef enum: uint16_t {
    kELFSectionHeaderNumberUndefined =  0,
    kELFSectionHeaderNumberAbsolute =   0xFFF1,
} kELFSectionHeaderNumber;

typedef enum: uint16_t {
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

typedef enum: uint8_t {
    kEmitModeNone,
    kEmitModeFirmware,
    kEmitModeRelocatableObject,
} kEmitMode;

#define R_EMEX64_ABS64  1

#define EI_NIDENT   16

#define ELF32_R_SYM(i) ((i) >> 32)
#define ELF32_R_TYPE(i) ((i) & 0xFFFFFFFF)
#define ELF64_R_INFO(s,t) (((uint64_t)(s) << 32) | (uint32_t)(t))

#define ELF_SYM_INFO(bind, type) (((bind) << 4) | ((type) & 0xf))

extern const uint8_t ident[EI_NIDENT];

#endif /* EMEX64LD_TYPE_H */
