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
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/file.h>

static inline int emex_file_policy_to_o_rw(EFFilePolicyPermission p)
{
    if((p & (kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite)) == (kEFFilePolicyPermissionRead | kEFFilePolicyPermissionWrite))
    {
        return O_RDWR;
    }
    if(p & kEFFilePolicyPermissionWrite)
    {
        return O_WRONLY;
    }
    return O_RDONLY;
}

static inline int emex_file_policy_to_prot(EFFilePolicyPermission p)
{
    int prot = PROT_NONE;
    prot |= ((p & kEFFilePolicyPermissionRead) ? PROT_READ : PROT_NONE);
    prot |= ((p & kEFFilePolicyPermissionWrite) ? PROT_WRITE : PROT_NONE);
    prot |= ((p & kEFFilePolicyPermissionExecute) ? PROT_EXEC : PROT_NONE);
    return prot;
}

static emex_file_t *__emex_file_alloc(const char *path,
                                      EFFilePolicy policy,
                                      Boolean care_about_file_exist_policy)
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
        if(policy.mustExist && care_about_file_exist_policy)
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
    f->type = emex_file_type_for_path(path, policy.mustExist);
    if(policy.mustBeAFile && f->type == kEFFileTypeDirectory)
    {
        free((void*)f->path);
        free(f);
        return NULL;
    }
    f->d = NULL;

    return f;
}

emex_file_t *emex_file_alloc(const char *path,
                             EFFilePolicy policy)
{
    return __emex_file_alloc(path, policy, true);
}

emex_file_t *emex_file_alloc_vfd(const char *path,
                                 EFFilePolicy policy,
                                EFFileHandleRef d)
{
    EFAUTOREL EFFileHandleRef handle = EFRetainTry(d);
    if(handle == NULL)
    {
        return NULL;
    }

    emex_file_t *f = __emex_file_alloc(path, policy, false);
    if(f == NULL)
    {
        return NULL;
    }

    f->type = emex_file_type_for_path(path, policy.mustExist);
    if(f->type == kEFFileTypeDirectory)
    {
        free(f);
        return NULL;
    }

    /* setting unsaved values */
    f->len = 0;
    f->content = MAP_FAILED;
    f->d = EFAUTOTRANSFER(handle);

    return f;
}

emex_file_t *emex_file_alloc_unsaved(const char *path,
                                     EFFilePolicy policy,
                                     const char *content)
{
    emex_file_t *f = __emex_file_alloc(path, policy, false);
    if(f == NULL)
    {
        return NULL;
    }

    f->type = emex_file_type_for_path(path, policy.mustExist);
    if(f->type == kEFFileTypeDirectory)
    {
        free(f);
        return NULL;
    }

    /* setting unsaved values */
    f->len = 0;
    f->content = MAP_FAILED;
    f->d = EFFileHandleCreate(kEFAllocatorDefault);
    if(f->d == NULL)
    {
        free(f->path);
        free(f);
        return NULL;
    }

    EFFileHandleWrite(f->d, (const UInt8*)content, (EFIndex)strlen(content));
    EFFileHandleSeek(f->d, 0, kEFFileHandleSeekTypeSet);

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

Boolean emex_file_open(emex_file_t *f)
{
    if(f->d != NULL)
    {
        return true;
    }

    if(f->type == kEFFileTypeDirectory)
    {
        return false;
    }

    /* initial open */
    EFAUTOREL EFStringRef pathStr = EFStringCreateWithCString(kEFAllocatorDefault, f->path, kEFStringEncodingUTF8);
    f->d = EFFileHandleCreateWithPathAndOptions(kEFAllocatorDefault, pathStr, emex_file_policy_to_o_rw(f->policy.neededPermission) | (f->policy.createOnOpen ? (O_CREAT | O_TRUNC) : 0), 0755);
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
        EFRelease(f->d);
        f->d = NULL;
    }
}

EFFileHandleRef emex_file_dup_vfd(emex_file_t *f)
{
    if(!emex_file_open(f))
    {
        return NULL;
    }
    return EFRetain(f->d);
}
EFBitWalkerRef emex_file_dup_vbitwalker(emex_file_t *f,
                                        EFEndian endian)
{
    if(!emex_file_open(f))
    {
        return NULL;
    }
    return EFBitWalkerCreateWithHandle(kEFAllocatorDefault, f->d, endian);
}

Boolean emex_file_map(emex_file_t *f)
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
    f->len = EFFileHandleGetLength(f->d);
    EFReleaseTry(f->map);
    f->map = EFFileHandleReadData(f->d, f->len);
    f->content = (char*)EFDataGetPtr(f->map);

    return true;
}

void emex_file_unmap(emex_file_t *f)
{
    if(f->content != MAP_FAILED)
    {
        EFReleaseTry(f->vpageObjRef);
        f->content = MAP_FAILED;
        f->len = 0;
    }
}

void emex_file_unlink(emex_file_t *f)
{
    if(EFFileHandleGetType(f->d) != kEFFileHandleTypeBSD)
    {
        unlink(f->path);
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

EFFileType emex_file_type_for_path(const char *path, Boolean must_exist)
{
    struct stat st;
    if(stat(path, &st) != 0)
    {
        if(!must_exist)
        {
            goto extension_validation;
        }

        return kEFFileTypeUnknown;
    }

    if(S_ISDIR(st.st_mode))
    {
        return kEFFileTypeDirectory;
    }
    else if(S_ISREG(st.st_mode))
extension_validation:
    {
        const char *extension = get_extension(path);
        if(strcmp("e64", extension) == 0)
        {
            return kEFFileTypeAssembly;
        }
        else if(strcmp("e64inc", extension) == 0)
        {
            return kEFFileTypeAssemblyIncludations; /* this shall be named kEFFileTypeAssemblyIncludation, without the s in the end */
        }
        else if(strcmp("c", extension) == 0)
        {
            return kEFFileTypeC;
        }
        else if(strcmp("h", extension) == 0)
        {
            return kEFFileTypeCHeader;
        }
        else if(strcmp("cpp", extension) == 0 ||
                strcmp("cxx", extension) == 0 ||
                strcmp("cc", extension) == 0)
        {
            return kEFFileTypeCXX;
        }
        else if(strcmp("hpp", extension) == 0)
        {
            return kEFFileTypeCXXHeader;
        }
        else if(strcmp("m", extension) == 0)
        {
            return kEFFileTypeObjC;
        }
        else if(strcmp("mm", extension) == 0)
        {
            return kEFFileTypeObjCXX;
        }
        else if(strcmp("o", extension) == 0)
        {
            return kEFFileTypeObject;
        }
    }

    /* couldn't resolve file type lol */
    return kEFFileTypeUnknown;
}

static Boolean __EFArrayAppendEmexFileCallback(void *ptr)
{
    return true;
}

static void __EFArrayRemoveEmexFileCallback(void *ptr)
{
    emex_file_dealloc((emex_file_t*)ptr);
}

static Boolean __EFArrayEqualEmexFileCallback(void *ptr1,
                                              void *ptr2)
{
    return (ptr1 == ptr2);
}

static EFStringRef __EFArrayCopyDescriptionEmexFileCallback(EFAllocatorRef allocatorRef,
                                                            void *ptr)
{
    return EFStringCreateWithFormat(allocatorRef, EFSTR("<emexfile %p>"), ptr);
}

EFArrayCallbacks kEFArrayCallbacksEmexFileCallbacks = &(struct EFArrayCallbacks){
    .append = __EFArrayAppendEmexFileCallback,
    .remove = __EFArrayRemoveEmexFileCallback,
    .equal = __EFArrayEqualEmexFileCallback,
    .copyDescription = __EFArrayCopyDescriptionEmexFileCallback,
};
