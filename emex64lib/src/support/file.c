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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <emex64lib/support/file.h>
#include <emex64lib/support/fdwalker.h>

emex_file_t *emex_file_alloc(const char *path)
{
    emex_file_t *f = malloc(sizeof(emex_file_t));
    if(f == NULL)
    {
        return NULL;
    }

    /*
     * resolving the true paths is important
     * so errors can reveal the actual file
     * locations.
     */
    char *tmp_path = malloc(PATH_MAX);
    if(realpath(path, tmp_path) == NULL)
    {
        free(tmp_path);
        free(f);
        return NULL;
    }
    f->path = tmp_path;
    

    /* setting standard values */
    f->instance_type = kEmexFileInstanceTypeSaved;
    f->len = 0;
    f->content = MAP_FAILED;
    f->type = emex_file_type_for_path(path, true);
    f->fd = -1;

    return f;
}

emex_file_t *emex_file_alloc_unsaved(const char *path,
                                     const char *content)
{
    emex_file_t *f = emex_file_alloc(path);
    if(f == NULL)
    {
        return NULL;
    }

    f->type = emex_file_type_for_path(path, false);
    if(f->type == kEmexFileTypeDirectory)
    {
        free(f);
        return NULL;
    }

    f->len = strlen(content);
    f->content = strdup(content);
    if(f->content == NULL)
    {
        free(f);
        return NULL;
    }

    /* setting unsaved values */
    f->instance_type = kEmexFileInstanceTypeUnsaved;
    f->fd = -1;
    f->content = MAP_FAILED;

    return f;
}

void emex_file_dealloc(emex_file_t *f)
{
    emex_file_unmap(f);
    emex_file_close(f);
    if(f->instance_type == kEmexFileInstanceTypeUnsaved)
    {
        free((void*)f->content);
    }
    free(f);
}

bool emex_file_open(emex_file_t *f)
{
    if(f->fd > 0)
    {
        emex_file_close(f);
    }

    if(f->type == kEmexFileTypeDirectory)
    {
        return false;
    }

    if(f->instance_type == kEmexFileInstanceTypeUnsaved)
    {
        /* TODO: open via creating the file at unsaved location flipping unsaved off */
        return false;
    }

    /* initial open */
    f->fd = open(f->path, O_RDONLY);
    if(f->fd < 0)
    {
        return false;
    }

    return true;
}

void emex_file_close(emex_file_t *f)
{
    close(f->fd);
    f->fd = -1;
}

bool emex_file_map(emex_file_t *f)
{
    if(f->content != MAP_FAILED)
    {
        emex_file_unmap(f);
    }

    /* initial open */
    if(!emex_file_open(f))
    {
        return false;
    }

    /* initially mapping assembly file */
    struct stat fdstat;
    if(fstat(f->fd, &fdstat) < 0)
    {
        return false;
    }

    f->len = fdstat.st_size;
    /* TODO: check if UTF8 encoded or force UTF8 encoding */
    f->content = mmap(NULL, f->len, PROT_READ | PROT_WRITE, MAP_PRIVATE, f->fd, 0);
    if(f->content == MAP_FAILED)
    {
        return false;
    }

    return true;
}

void emex_file_unmap(emex_file_t *f)
{
    if(f->instance_type == kEmexFileInstanceTypeUnsaved && f->content != MAP_FAILED)
    {
        munmap((void*)f->content, f->len);
        f->content = MAP_FAILED;
        f->len = 0;
    }
}

static inline const char *get_extension(const char *path)
{
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;

    const char *dot = strrchr(base, '.');
    if(!dot || dot == base)
    {
        return "";
    }
    return dot + 1;
}

kEmexFileType emex_file_type_for_path(const char *path, bool must_exist)
{
    struct stat st;
    if(stat(path, &st) != 0)
    {
        if(!must_exist)
        {
            goto extension_validation;
        }

        perror("stat");
        return kEmexFileTypeUnknown;
    }

    if(S_ISDIR(st.st_mode))
    {
        return kEmexFileTypeDirectory;
    }
    else if(S_ISREG(st.st_mode))
extension_validation:
    {
        const char *extension = get_extension(path);
        if(strcmp("e64", extension) == 0)
        {
            return kEmexFileTypeAssembly;
        }
        else if(strcmp("e64inc", extension) == 0)
        {
            return kEmexFileTypeAssemblyIncludation;
        }
        else if(strcmp("c", extension) == 0)
        {
            return kEmexFileTypeC;
        }
        else if(strcmp("h", extension) == 0)
        {
            return kEmexFileTypeCHeader;
        }
        else if(strcmp("cpp", extension) == 0 ||
                strcmp("cxx", extension) == 0 ||
                strcmp("cc", extension) == 0)
        {
            return kEmexFileTypeCXX;
        }
        else if(strcmp("hpp", extension) == 0)
        {
            return kEmexFileTypeCXXHeader;
        }
        else if(strcmp("m", extension) == 0)
        {
            return kEmexFileTypeObjC;
        }
        else if(strcmp("mm", extension) == 0)
        {
            return kEmexFileTypeObjCXX;
        }
        else if(strcmp("o", extension) == 0)
        {
            return kEmexFileTypeObject;
        }
    }

    /* couldn't resolve file type lol */
    return kEmexFileTypeUnknown;
}
