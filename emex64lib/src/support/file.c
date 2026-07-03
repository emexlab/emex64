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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <emex64lib/support/file.h>

emex_file_policy_t in_data_file_policy = {
    .needed_permission = kEmexFilePolicyPermissionRead,
    .must_exist = true,
    .must_be_file = true,
    .create_on_open = false,
};

emex_file_policy_t out_data_file_policy = {
    .needed_permission = kEmexFilePolicyPermissionRead | kEmexFilePolicyPermissionWrite,
    .must_exist = false,
    .must_be_file = true,
    .create_on_open = true,
};

emex_file_policy_t out_nocreate_file_policy = {
    .needed_permission = kEmexFilePolicyPermissionRead | kEmexFilePolicyPermissionWrite,
    .must_exist = false,
    .must_be_file = true,
    .create_on_open = false,
};

static inline int emex_file_policy_to_o_rw(kEmexFilePolicyPermission p)
{
    if((p & (kEmexFilePolicyPermissionRead | kEmexFilePolicyPermissionWrite)) == (kEmexFilePolicyPermissionRead | kEmexFilePolicyPermissionWrite))
    {
        return O_RDWR;
    }
    if(p & kEmexFilePolicyPermissionWrite)
    {
        return O_WRONLY;
    }
    return O_RDONLY;
}

static inline int emex_file_policy_to_prot(kEmexFilePolicyPermission p)
{
    int prot = PROT_NONE;
    prot |= ((p & kEmexFilePolicyPermissionRead) ? PROT_READ : PROT_NONE);
    prot |= ((p & kEmexFilePolicyPermissionWrite) ? PROT_WRITE : PROT_NONE);
    prot |= ((p & kEmexFilePolicyPermissionExecute) ? PROT_EXEC : PROT_NONE);
    return prot;
}

static emex_file_t *__emex_file_alloc(const char *path,
                                      emex_file_policy_t policy,
                                      bool care_about_file_exist_policy)
{
    emex_file_t *f = malloc(sizeof(emex_file_t));
    if(f == NULL)
    {
        return NULL;
    }

    f->policy = policy;

    /*
     * resolving the true paths is important
     * so errors can reveal the actual file
     * locations.
     */
    char *tmp_path = malloc(PATH_MAX);
    if(realpath(path, tmp_path) == NULL)
    {
        if(policy.must_exist && care_about_file_exist_policy)
        {
            free(tmp_path);
            free(f);
            return NULL;
        }

        free(tmp_path);
        tmp_path = strdup(path);
    }
    f->path = tmp_path;
    

    /* setting standard values */
    f->len = 0;
    f->content = MAP_FAILED;
    f->type = emex_file_type_for_path(path, policy.must_exist);
    if(policy.must_be_file && f->type == kEmexFileTypeDirectory)
    {
        free((void*)f->path);
        free(f);
        return NULL;
    }
    f->d = NULL;

    return f;
}

emex_file_t *emex_file_alloc(const char *path,
                             emex_file_policy_t policy)
{
    return __emex_file_alloc(path, policy, true);
}

emex_file_t *emex_file_alloc_vfd(const char *path,
                                 emex_file_policy_t policy,
                                 vfd_t *d)
{
    d = vfd_dup(d);
    if(d == NULL)
    {
        return NULL;
    }

    emex_file_t *f = __emex_file_alloc(path, policy, false);
    if(f == NULL)
    {
        vfd_close(d);
        return NULL;
    }

    f->type = emex_file_type_for_path(path, policy.must_exist);
    if(f->type == kEmexFileTypeDirectory)
    {
        vfd_close(d);
        free(f);
        return NULL;
    }

    /* setting unsaved values */
    f->len = 0;
    f->content = MAP_FAILED;
    f->d = d;

    return f;
}

emex_file_t *emex_file_alloc_unsaved(const char *path,
                                     emex_file_policy_t policy,
                                     const char *content)
{
    emex_file_t *f = __emex_file_alloc(path, policy, false);
    if(f == NULL)
    {
        return NULL;
    }

    f->type = emex_file_type_for_path(path, policy.must_exist);
    if(f->type == kEmexFileTypeDirectory)
    {
        free(f);
        return NULL;
    }

    /* setting unsaved values */
    f->len = 0;
    f->content = MAP_FAILED;
    f->d = vfd_vopen();
    if(f->d == NULL)
    {
        free(f->path);
        free(f);
        return NULL;
    }

    vfd_write(f->d, content, strlen(content));
    vfd_seek(f->d, 0, SEEK_SET);

    return f;
}

void emex_file_dealloc(emex_file_t *f)
{
    if(f == NULL)
    {
        return;
    }

    emex_file_unmap(f);
    emex_file_close(f);
    free(f->path);
    free(f);
}

bool emex_file_open(emex_file_t *f)
{
    if(f->d != NULL)
    {
        return true;
    }

    if(f->type == kEmexFileTypeDirectory)
    {
        return false;
    }

    /* initial open */
    f->d = vfd_open(f->path, emex_file_policy_to_o_rw(f->policy.needed_permission) | (f->policy.create_on_open ? (O_CREAT | O_TRUNC) : 0), 0755);
    if(f->d == NULL)
    {
        return false;
    }

    return true;
}

void emex_file_close(emex_file_t *f)
{
    if(f->d != NULL)
    {
        vfd_close(f->d);
        f->d = NULL;
    }
}

vfd_t *emex_file_dup_vfd(emex_file_t *f)
{
    if(!emex_file_open(f))
    {
        return NULL;
    }
    return vfd_dup(f->d);
}

vbitwalker_t *emex_file_dup_vbitwalker(emex_file_t *f,
                                       bw_endian_t endian)
{
    if(!emex_file_open(f))
    {
        return NULL;
    }

    return vbitwalker_alloc(f->d, endian);
}

bool emex_file_map(emex_file_t *f)
{
    if(f->content != MAP_FAILED)
    {
        /*
         * there could be a reason to remap,
         * for example file contents that changed.
         */
        emex_file_unmap(f);
    }

    /* initial open */
    if(!emex_file_open(f))
    {
        return false;
    }

    /* initially mapping assembly file */
    struct stat fdstat;
    if(vfd_stat(f->d, &fdstat) < 0)
    {
        return false;
    }

    f->len = fdstat.st_size;
    /* TODO: check if UTF8 encoded or force UTF8 encoding */
    switch(f->d->type)
    {
        case kVFDTypeReal:
        {
            vpage_t *p = __vpage_alloc(NULL, f->len, emex_file_policy_to_prot(f->policy.needed_permission), MAP_SHARED, f->d->fd, 0);
            if(p == NULL)
            {
                return false;
            }

            f->vpageObjRef = VpageObjCreateWithVpage(kEFAllocatorDefault, p);
            if(f->vpageObjRef == NULL)
            {
                vpage_dealloc(p);
                return false;
            }
            break;
        }
        case kVFDTypeVirtual:
        {
            if(!EFRetain(f->d->vd.vpageObjRef))
            {
                return false;
            }

            VpageObjRef *vpageObjRef = f->d->vd.vpageObjRef;
            if(!VpageObjMergePage(vpageObjRef))
            {
                EFRelease(vpageObjRef);
                return false;
            }

            f->vpageObjRef = vpageObjRef;
            break;
        }
    }


    vpage_t *vpage = VpageObjGetVpage(f->vpageObjRef);
    f->content = (char*)vpage->p;
    f->len = vpage->len;

    return true;
}

void emex_file_unmap(emex_file_t *f)
{
    if(f->content != MAP_FAILED)
    {
        EFRelease(f->vpageObjRef);
        f->content = MAP_FAILED;
        f->len = 0;
    }
}

void emex_file_unlink(emex_file_t *f)
{
    if(f->d != NULL && f->d->type == kVFDTypeVirtual)
    {
        /* is virtual anyways */
        return;
    }
    
    unlink(f->path);
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
