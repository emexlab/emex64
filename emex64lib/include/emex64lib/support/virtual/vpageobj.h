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

#ifndef EMEX64_VPAGEOBJ_H
#define EMEX64_VPAGEOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <EmexFoundation/EmexFoundation.h>
#include <emex64lib/support/virtual/vpage.h>

typedef EFObjectRef VpageObjRef;

EFTypeID VpageObjGetType(void);

VpageObjRef VpageObjCreate(EFAllocatorRef allocatorRef);
VpageObjRef VpageObjCreateWithVpage(EFAllocatorRef allocatorRef, vpage_t *vpage);

vpage_t *VpageObjGetVpage(VpageObjRef vpageObjRef);

size_t VpageObjGetSize(VpageObjRef vpageObjRef);
bool VpageObjExtendPage(VpageObjRef vpageObjRef);
bool VpageObjMergePage(VpageObjRef vpageObjRef);

size_t VpageObjWrite(VpageObjRef vpageObjRef, size_t off, const uint8_t *b, size_t len);
size_t VpageObjRead(VpageObjRef vpageObjRef, size_t off, uint8_t *b, size_t len);

/* need to be replaced */
size_t VpageObjGetEndMarker(VpageObjRef vpageObjRef);
void VpageObjSetEndMarker(VpageObjRef vpageObjRef, size_t endMarker);

#endif /* EMEX64_VPAGEOBJ_H */
