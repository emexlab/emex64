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

#ifndef EMEX64VM_MMU_H
#define EMEX64VM_MMU_H

#include <stdbool.h>
#include <stdint.h>
#include <emex64vm/core.h>

#define LA64_MMU_PAGE_SIZE          8192   /* la64 uses 8K pages (I just desided that because my kernel uses 8K pages already and im too lazy to move to 4K pages) */

/* page table entry flags */
#define LA64_MMU_PT_PRESENT         0b00000001
#define LA64_MMU_PT_USER            0b00000010
#define LA64_MMU_PT_DIRTY           0b00000100
#define LA64_MMU_PT_READ            0b00001000
#define LA64_MMU_PT_WRITE           0b00010000
#define LA64_MMU_PT_EXEC            0b00100000

/* page table enry masks */
#define LA64_MMU_MASK_FLAGS         0b0000000000000000000000000000000000000000000000000000000011111111
#define LA64_MMU_MASK_PFN           0b1111111111111111111111111111111111111111111111111111111100000000

/* access types */
#define LA64_MMU_ACC_NONE           0
#define LA64_MMU_ACC_READ           LA64_MMU_PT_READ
#define LA64_MMU_ACC_WRITE          LA64_MMU_PT_WRITE
#define LA64_MMU_ACC_EXEC           LA64_MMU_PT_EXEC

bool la64_mmu_access(la64_core_t *core, uint64_t vaddr, uint8_t acc, uint64_t *paddr);

#endif /* EMEX64VM_MMU_H */
