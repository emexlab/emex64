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

#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/mmio.h>
#include <emex64lib/vm/mmu.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

la64_memory_t *la64_memory_alloc(uint64_t size)
{
    /*
     * allocating random access memory, which
     * must be aligned to page size for the
     * sake of god.
     */
    la64_memory_t *memory = malloc(sizeof(la64_memory_t));
    if(memory == NULL)
    {
        return NULL;
    }

    /* allocate raw memory (using mmap for larger sizes, better than heap in this case) */
    memory->memory_size = LA64_PAGE_ROUND_UP(size);
    memory->memory = mmap(NULL, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory->memory == MAP_FAILED)
    {
        free(memory);
        return NULL;
    }

    return memory;
}

void la64_memory_dealloc(la64_memory_t *memory)
{
    munmap(memory->memory, memory->memory_size);
    free(memory);
}

bool la64_memory_load_image(la64_memory_t *memory,
                            const char *image_path)
{
    /*
     * opening bios image with RO(read-only)
     * access, which is because we don't
     * write to it and we shall not write
     * to it.
     */
    int fd = open(image_path, O_RDONLY);
    if(fd == -1)
    {
        printf("[vm] failed to open bios image at path %s\n", image_path);
        return false;
    }

    /* gather information of bios image  */
    struct stat image_stat;
    if(fstat(fd, &image_stat) != 0)
    {
        printf("[vm] failed to gather size of file at path %s\n", image_path);
        return false;
    }

    size_t image_size = LA64_PAGE_ROUND_UP(image_stat.st_size);
    if(image_size > memory->memory_size)
    {
        printf("[vm] error: bios image is too large\n");
        return false;
    }

    /*
     * writing bios image by mapping it
     * and writing it back.
     */
    void *image_mapped = mmap(NULL, image_size, PROT_READ, MAP_SHARED, fd, 0);
    if(image_mapped == MAP_FAILED)
    {
        printf("[vm] error: failed to map bios image\n");
        return false;
    }

    memcpy(memory->memory, image_mapped, image_size);
    munmap(image_mapped, image_size);

    close(fd);

    return true;
}

void *la64_memory_access(la64_core_t *core,
                         uint64_t addr,
                         size_t size)
{
    assert(size != 0);

    uint64_t addr_end = addr + size;
    
    if(addr >= addr_end ||
       core->machine->memory->memory_size < addr_end)
    {
        /* attempt to access memory is OOB. */
        return NULL;
    }

    return &(core->machine->memory->memory[addr]);
}

bool la64_memory_read(la64_core_t *core,
                      uint64_t addr,
                      size_t size,
                      uint64_t *value)
{
    if(!la64_mmu_access(core, addr, LA64_MMU_ACC_READ, &addr))
    {
        return false;
    }

    /* MMIO devices exist ^^ */
    la64_mmio_region_t *mmio = la64_mmio_find(core->machine->mmio_bus, addr);
    if(mmio != NULL)
    {
        if(mmio->read != NULL)
        {
            *value = mmio->read(core, mmio->device, addr - mmio->base_addr, (int)size);
            return true;
        }
        return false;
    }

    void *ptr = la64_memory_access(core, addr, size);
    if(ptr == NULL)
    {
        return false;
    }

    switch(size)
    {
        case 1:
            *value = *(uint8_t *)ptr;
            return true;
        case 2:
            *value = *(uint16_t *)ptr;
            return true;
        case 4:
            *value = *(uint32_t *)ptr;
            return true;
        case 8:
            *value = *(uint64_t *)ptr;
            return true;
        default:
            return false;
    }
}

bool la64_memory_write(la64_core_t *core,
                       uint64_t addr,
                       uint64_t value,
                       size_t size)
{
    if(!la64_mmu_access(core, addr, LA64_MMU_ACC_WRITE, &addr))
    {
        return false;
    }

    /* MMIO devices exist ^^ */
    la64_mmio_region_t *mmio = la64_mmio_find(core->machine->mmio_bus, addr);
    if(mmio != NULL)
    {
        if(mmio->write != NULL)
        {
            mmio->write(core, mmio->device, addr - mmio->base_addr, value, (int)size);
            return true;
        }
        return false;
    }

    void *ptr = la64_memory_access(core, addr, size);
    if(ptr == NULL)
    {
        return false;
    }

    switch(size)
    {
        case 1:
            *(uint8_t *)ptr = (uint8_t)value;
            return true;
        case 2:
            *(uint16_t *)ptr = (uint16_t)value;
            return true;
        case 4:
            *(uint32_t *)ptr = (uint32_t)value;
            return true;
        case 8:
            *(uint64_t *)ptr = value;
            return true;
        default:
            return false;
    }
}
