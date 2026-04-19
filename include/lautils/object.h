/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LAUTILS_OBJECT_H
#define LAUTILS_OBJECT_H

#include <stdint.h>

/**** HEADER MACROS ****/

#define LAO_MAGIC        0xFADEBACE

#define LAO_VERSION_NEWEST      0x0 /* TODO: create support check macros for the version field */
#define LAO_VERSION_OLDEST      0x0

#define LAO_ARCH_NONE           0x0
#define LAO_ARCH_LA16           0x1 /* unsupported yet */
#define LAO_ARCH_LA32           0x2 /* unsupported yet */
#define LAO_ARCH_LA64           0x3

#define LAO_ABI_NONE            0x0
#define LAO_ABI_BOOTIMG         LAO_ABI_NONE /* bootimg means without ABI, there is no ABI as there is no proper operating system yet */

#define LAO_TYPE_NONE           0b000000000
#define LAO_TYPE_OBJECT         0b000000001
#define LAO_TYPE_EXECUTABLE     0b000000010
#define LAO_TYPE_LIBRARY        (LAO_TYPE_OBJECT | LAO_TYPE_EXECUTABLE) /* aka shared object */

#define LAO_BASE_HEADER_MAGIC_VALID(header) (header->magic == LAO_MAGIC)
#define LAO_BASE_HEADER_VERSION_VALID(header) (header->version == LAO_VERSION_NEWEST)
#define LAO_BASE_HEADER_ARCH_VALIC(header) (header->arch == LAO_ARCH_LA64)
#define LAO_BASE_HEADER_ABI_VALID(header) (header->abi == LAO_ABI_BOOTIMG)
#define LAO_BASE_HEADER_TYPE_VALID(header) (header->type == LAO_TYPE_OBJECT || header->type == LAO_TYPE_EXECUTABLE) /* once library support is there we can make truly use out of the bitmask */

#define LAO_BASE_HEADER_VALID(header) (LAO_BASE_HEADER_MAGIC_VALID(header) && LAO_BASE_HEADER_VERSION_VALID(header) && LAO_BASE_HEADER_ARCH_VALID(header) && LAO_BASE_HEADER_ABI_VALID(header))

/**** SECTION MACROS ****/

#define LAO_SECTION_TYPE_NONE    0x0
#define LAO_SECTION_TYPE_DATA    0x1        /* is a data section for all kinds of data, executable, ro, and so on */
#define LAO_SECTION_TYPE_BSS     0x2        /* is aways writable and got no offset cuz its a stack allocated region of memory */

#define LAO_SECTION_PROT_NONE    0b00000000
#define LAO_SECTION_PROT_READ    0b00000001
#define LAO_SECTION_PROT_WRITE   0b00000010
#define LAO_SECTION_PROT_EXEC    0b00000100 /* for example for executable memory a section would be data and executable */
#define LAO_SECTION_PROT_USER    0b00001000 /* for kernels */
#define LAO_SECTION_PROT_KTRR    0b00010000 /* Kernel Text Read-Only Region (a comming soon features of LA64) */
#define LAO_SECTION_PROT_ALL     0b00011111 /* for validity checks */

/**** STRUCTURES ****/

/* MARK: symbol table */

typedef struct __attribute__((packed)) lao_symbol_entry {
    uint64_t name_offset;
    uint64_t symbol_offset;
} lao_symbol_entry_t;

typedef struct __attribute__((packed)) lao_symbol_table {
    uint32_t count;
    /* symbol entrie's start right after */
} lao_symbol_table_t;

/* MARK: reloc table */

typedef struct __attribute__((packed)) lao_reloc_table_entry {
    uint32_t symbol_index;
    uint32_t placeholder_offset;
} lao_reloc_table_entry_t;

typedef struct __attribute__((packed)) lao_reloc_table {
    uint32_t count;
    /* reloc entrie's start right after */
} lao_reloc_table_t;

/* MARK: section table */

typedef struct __attribute__((packed)) lao_section_table_entry {
    uint8_t type;
    uint8_t prot;
    uint64_t size;
} lao_section_table_entry_t;

typedef struct __attribute__((packed)) lao_section_table {
    uint32_t count;
    /* section entrie's start right after */
} lao_section_table_t;

/* MARK: header */

typedef struct __attribute__((packed)) lao_base_header {
    uint32_t magic;
    uint8_t version;
    uint8_t arch;
    uint8_t abi;
    uint8_t type;
    /* header starts right after */
} lao_base_header_t;

typedef struct __attribute__((packed)) lao_header64 {
    uint64_t string_table_offset;
    uint64_t symbol_table_offset;
    uint64_t section_table_offset;
    uint64_t reloc_table_offset;
    uint64_t start_offset;          /* in executables that is the CPU's "I jump there" offset */
} lao_header64_t;

#endif /* LAUTILS_OBJECT_H */

