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

#include <sys/mman.h>

#include <emex64lib/support/virtual/vpage.h>
#include <emex64lib/vm/memory.h>

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

vpage_t *vpage_alloc()
{
    vpage_t *p = calloc(1, sizeof(vpage_t));
    p->len = EMEX64_PAGE_SIZE;
    p->p = mmap(NULL, p->len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(p->p == MAP_FAILED)
    {
        free(p);
        return NULL;
    }
    return p;
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

bool vpage_gib_page(vpage_t *p)
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

bool vpage_bind_page(vpage_t *p)
{
    vpage_t *page = vpage_get_first(p);
    if(page->next == NULL)
    {
        return true;
    }

    /* allocating new map */
    size_t total_len = vpage_get_size(page);
    uint8_t *newmap = mmap(NULL, total_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
                         uint8_t *b,
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

size_t vpage_write(vpage_t *p, size_t off, const uint8_t *b, size_t len)
{
    return vpage_xfer(p, off, (uint8_t *)b, len, kVPXferWrite);
}

size_t vpage_read(vpage_t *p, size_t off, uint8_t *b, size_t len)
{
    return vpage_xfer(p, off, b, len, kVPXferRead);
}
