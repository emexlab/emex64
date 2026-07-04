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

#include <pthread.h>
#include <assert.h>
#include <EmexToolchain/support/virtual/vpageobj.h>

typedef struct VpageObj {
    EFObject header;
    vpage_t *root;
    size_t extra_size_marker;
} *VpageObj;

void __VpageObjDeinit(VpageObjRef ref)
{
    VpageObj obj = (VpageObj)ref;
    if(obj->root != NULL)
    {
        vpage_dealloc(obj->root);
    }
}

EFClass VpageObjClass = {
    .name = "VpageObj",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __VpageObjDeinit,
};

static void VpageObjRegisterClass(void)
{
    EFClassRegister(&VpageObjClass);
}

EFTypeID VpageObjGetType(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, VpageObjRegisterClass);
    return VpageObjClass.typeID;
}

VpageObjRef VpageObjCreate(EFAllocatorRef allocatorRef)
{
    vpage_t *vpage = vpage_alloc();
    if(vpage == NULL)
    {
        return NULL;
    }

    return VpageObjCreateWithVpage(allocatorRef, vpage);
}

VpageObjRef VpageObjCreateWithVpage(EFAllocatorRef allocatorRef,
                                    vpage_t *vpage)
{
    assert(vpage != NULL);

    VpageObj obj = EFObjectAlloc(allocatorRef, VpageObjGetType(), sizeof(struct VpageObj));
    if(obj == NULL)
    {
        return NULL;
    }

    obj->root = vpage;
    obj->extra_size_marker = 0;

    return obj;
}

vpage_t *VpageObjGetVpage(VpageObjRef vpageObjRef)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return obj->root;
}

size_t VpageObjGetSize(VpageObjRef vpageObjRef)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return vpage_get_size(obj->root);
}

bool VpageObjExtendPage(VpageObjRef vpageObjRef)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return vpage_gib_page(obj->root);
}

bool VpageObjMergePage(VpageObjRef vpageObjRef)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return vpage_bind_page(obj->root);
}

size_t VpageObjWrite(VpageObjRef vpageObjRef,
                     size_t off,
                     const uint8_t *b,
                     size_t len)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return vpage_write(obj->root, off, b, len);
}

size_t VpageObjRead(VpageObjRef vpageObjRef,
                    size_t off,
                    uint8_t *b,
                    size_t len)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return vpage_read(obj->root, off, b, len);
}

size_t VpageObjGetEndMarker(VpageObjRef vpageObjRef)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    return obj->extra_size_marker;
}

void VpageObjSetEndMarker(VpageObjRef vpageObjRef,
                          size_t endMarker)
{
    VpageObj obj = (VpageObj)vpageObjRef;
    obj->extra_size_marker = endMarker;
}
