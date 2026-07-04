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

#include <sys/mman.h>
#include <EmexToolchain/support/virtual/vpage.h>
#include <EmexToolchain/vm/E64Memory.h>

static vpage_t *vpage_get_first(vpage_t *p)
{
    vpage_t *page = p;
    for(;;)
    {
        if(page->prev == NULL)
        {
            break;
        }

        page = page->prev;
    }
    return page;
}

static vpage_t *vpage_get_last(vpage_t *p)
{
    vpage_t *page = p;
    for(;;)
    {
        if(page->next == NULL)
        {
            break;
        }

        page = page->next;
    }
    return page;
}

void *__vpage_alloc(void *addr,
                    size_t len,
                    int prot,
                    int flags,
                    int fd,
                    off_t offset)
{
    vpage_t *p = calloc(1, sizeof(vpage_t));
    if(p == NULL)
    {
        return NULL;
    }
    
    p->len = len;
    p->p = mmap(addr, len, prot, flags, fd, offset);
    if(p->p == MAP_FAILED)
    {
        free(p);
        return NULL;
    }

    return p;
}

vpage_t *vpage_alloc()
{
    return __vpage_alloc(NULL, EMEX64_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static void __vpage_dealloc(vpage_t *p)
{
    munmap(p->p, p->len);
    free(p);
    return;
}

void vpage_dealloc(vpage_t *p)
{
    vpage_t *page = vpage_get_first(p);
    while(page != NULL)
    {
        vpage_t *next = page->next;
        __vpage_dealloc(page);
        page = next;
    }
    return;
}

size_t vpage_get_size(vpage_t *p)
{
    vpage_t *page = vpage_get_first(p);
    size_t len = 0;
    for(;;)
    {
        len += page->len;

        if(page->next == NULL)
        {
            break;
        }

        page = page->next;
    }
    return len;
}

Boolean vpage_gib_page(vpage_t *p)
{
    vpage_t *page = vpage_get_last(p);
    vpage_t *new = vpage_alloc();
    if(new == NULL)
    {
        return false;
    }
    page->next = new;
    new->prev = page;
    return true;
}

Boolean vpage_bind_page(vpage_t *p)
{
    vpage_t *page = vpage_get_first(p);
    if(page->next == NULL)
    {
        return true;
    }

    /* allocating new map */
    size_t total_len = vpage_get_size(page);
    UInt8 *newmap = mmap(NULL, total_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(newmap == MAP_FAILED)
    {
        return false;
    }

    /* copying data from old pages to single page */
    vpage_t *oldpage = page;
    size_t loc = 0;
    for(;;)
    {
        memcpy(newmap + loc, page->p, page->len);

        vpage_t *next = page->next;
        size_t add_len = page->len;
        if(loc > 0)
        {
            __vpage_dealloc(page);
        }
        loc += add_len;

        if(next == NULL)
        {
            break;
        }

        page = next;
    }

    /* setting to new map */
    oldpage->next = NULL;
    oldpage->p = newmap;
    oldpage->len = total_len;

    return true;
}

typedef enum {
    kVPXferRead,
    kVPXferWrite
} kVPXfer;

static size_t vpage_xfer(vpage_t *p,
                         size_t off,
                         UInt8 *b,
                         size_t len,
                         kVPXfer xfer)
{
    vpage_t *page = vpage_get_first(p);
    size_t total  = vpage_get_size(page);

    /* avoiding overflow */
    if(off >= total)
    {
        return 0;
    }
    if(len > total - off)
    {
        len = total - off;
    }

    /* walk to the page that contains the starting offset */
    size_t base = 0;
    while(page != NULL && base + page->len <= off)
    {
        base += page->len;
        page = page->next;
    }

    size_t done = 0;
    while(len > 0 && page != NULL)
    {
        size_t page_off = off - base;
        size_t avail = page->len - page_off;
        size_t n = (len < avail) ? len : avail;

        if(xfer == kVPXferWrite)
        {
            memcpy(page->p + page_off, b + done, n);
        }
        else
        {
            memcpy(b + done, page->p + page_off, n);
        }

        done += n;
        len -= n;
        off += n;
        base += page->len;
        page = page->next;
    }

    return done;
}

size_t vpage_write(vpage_t *p, size_t off, const UInt8 *b, size_t len)
{
    return vpage_xfer(p, off, (UInt8 *)b, len, kVPXferWrite);
}

size_t vpage_read(vpage_t *p, size_t off, UInt8 *b, size_t len)
{
    return vpage_xfer(p, off, b, len, kVPXferRead);
}
