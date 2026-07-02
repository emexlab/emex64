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

#include <pthread.h>
#include <assert.h>
#include <emex64lib/support/virtual/vpageobj.h>

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
