/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#ifndef EMEXUTILS_OBJECT_H
#define EMEXUTILS_OBJECT_H

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
#define LAO_BASE_HEADER_ARCH_VALID(header) (header->arch == LAO_ARCH_LA64)
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

#define LAO_SECTION_MAP_NONE     0x0 /* maps nothing */
#define LAO_SECTION_MAP_FIXED    0x1 /* maps to a fixed address */
#define LAO_SECTION_MAP_RANDOM   0x2 /* maps at a random address within the available address space of the architecture in LA64 that is within a 53bit address space */
#define LAO_SECTION_MAP_ALIGN    0x3 /* maps alligned next to the last mapped page */
#define LAO_SECTION_MAP_DEFAULT  LAO_SECTION_MAP_ALIGN

/**** SYMBOL MACROS ****/

#define LAO_SYMBOL_TYPE_DEFAULT 0x0         /* normal ass code symbol */
#define LAO_SYMBOL_TYPE_BSS     0x1         /* references to a BSS section */

/**** RELOCATION MACROS ****/

#define LAO_RELOC_TYPE_ABSOLUTE 0x0         /* absoloute offset in image */
#define LAO_RELOC_TYPE_REL2PAGE 0x1         /* takes page start and appends address to it */
#define LAO_RELOC_TYPE_GOT      0x2         /* ignored by static linker, but not by dynamic linker */

/**** TYPES ****/

typedef uint16_t off16_t;
typedef uint32_t off32_t;
typedef uint64_t off64_t;

typedef uint16_t idx16_t;
typedef uint32_t idx32_t;

/**** STRUCTURES ****/

/* MARK: symbol table */

typedef struct __attribute__((packed)) lao_symbol_entry {
    uint8_t type;
    idx32_t name_index;
    off64_t symbol_offset;
    idx32_t section_index;
    off32_t symbol_size; /* matters only for BSS, BSS is fairly dynamic, the map type can only change how BSS regions are mapped in the first place */
    uint8_t  _pad[3]; 
} lao_symbol_entry_t;

typedef struct __attribute__((packed)) lao_symbol_table {
    idx32_t count;
    uint8_t  _pad[4];
    /* symbol entrie's start right after */
} lao_symbol_table_t;

/* MARK: reloc table */

typedef struct __attribute__((packed)) lao_reloc_table_entry {
    idx32_t symbol_index;
    off64_t placeholder_offset;
    uint8_t type;
    uint8_t  _pad[3];
} lao_reloc_table_entry_t;

typedef struct __attribute__((packed)) lao_reloc_table {
    idx32_t count;
    uint8_t  _pad[4];
    /* reloc entrie's start right after */
} lao_reloc_table_t;

/* MARK: section table */

typedef struct __attribute__((packed)) lao_section_table_entry {
    uint8_t type;
    uint8_t prot;
    uint8_t map;
    uint8_t  _pad0[5];
    off64_t vaddr;  /* if fixed the page will load at that specific location */
    off64_t start;
    off64_t size;
} lao_section_table_entry_t;

typedef struct __attribute__((packed)) lao_section_table {
    idx32_t count;
    uint8_t  _pad[4];
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
    off64_t symbol_table_offset;
    off64_t section_table_offset;
    off64_t reloc_table_offset;
    off64_t start_offset;          /* in executables that is the CPU's "I jump there" offset */
    idx32_t string_table_pages_count;
    uint8_t  _pad[4];
    /* string table pages start right after and after string table the other tables and data */
} lao_header64_t;

/**** NONO Assertions ****/

_Static_assert(sizeof(lao_symbol_entry_t) == 24, "lao_symbol_entry size");
_Static_assert(sizeof(lao_symbol_table_t) == 8,  "lao_symbol_table size");
_Static_assert(sizeof(lao_reloc_table_entry_t) == 16, "lao_reloc_table_entry size");
_Static_assert(sizeof(lao_reloc_table_t) == 8,  "lao_reloc_table size");
_Static_assert(sizeof(lao_section_table_entry_t) == 32, "lao_section_table_entry size");
_Static_assert(sizeof(lao_section_table_t) == 8,  "lao_section_table size");
_Static_assert(sizeof(lao_base_header_t) == 8,  "lao_base_header size");
_Static_assert(sizeof(lao_header64_t) == 40, "lao_header64 size");

#endif /* EMEXUTILS_OBJECT_H */
