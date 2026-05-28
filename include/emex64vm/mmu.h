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

/* page table entry flags */
#define LA64_MMU_PT_PRESENT         0b00000001  /* marks a PTE or PXD as present */
#define LA64_MMU_PT_USER            0b00000010  /* marks a PTE as user accessible, meaning user mode can access that page */
#define LA64_MMU_PT_DIRTY           0b00000100  /* marks a PTE as dirty, writes on it cause a page fault TODO: to be implemented */
#define LA64_MMU_PT_READ            0b00001000  /* marks a PTE as readable */
#define LA64_MMU_PT_WRITE           0b00010000  /* marks a PTE as writable (most MMU's don't have that, but this one does) */
#define LA64_MMU_PT_EXEC            0b00100000  /* marks a PTE as executable (means the CPU core can fetch instructions from it and execute them) */

/* page table enry masks */
#define LA64_MMU_MASK_FLAGS         0b0000000000000000000000000000000000000000000000000000000011111111
#define LA64_MMU_MASK_PFN           0b1111111111111111111111111111111111111111111111111111111100000000

/* access types */
#define LA64_MMU_ACC_PXD            0   /* MARK: this is a internal private type, don't use it outside of mmu.c */
#define LA64_MMU_ACC_READ           LA64_MMU_PT_READ
#define LA64_MMU_ACC_WRITE          LA64_MMU_PT_WRITE
#define LA64_MMU_ACC_EXEC           LA64_MMU_PT_EXEC

bool la64_mmu_access(la64_core_t *core, uint64_t vaddr, uint8_t acc, uint64_t *paddr);

#endif /* EMEX64VM_MMU_H */
