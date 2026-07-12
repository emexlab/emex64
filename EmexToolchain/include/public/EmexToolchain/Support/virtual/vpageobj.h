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

#ifndef EMEX64_VPAGEOBJ_H
#define EMEX64_VPAGEOBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/virtual/vpage.h>

typedef EFObjectRef VpageObjRef;

EFTypeID VpageObjGetType(void);

VpageObjRef VpageObjCreate(EFAllocatorRef allocatorRef);
VpageObjRef VpageObjCreateWithVpage(EFAllocatorRef allocatorRef, vpage_t *vpage);

vpage_t *VpageObjGetVpage(VpageObjRef vpageObjRef);

size_t VpageObjGetSize(VpageObjRef vpageObjRef);
Boolean VpageObjExtendPage(VpageObjRef vpageObjRef);
Boolean VpageObjMergePage(VpageObjRef vpageObjRef);

size_t VpageObjWrite(VpageObjRef vpageObjRef, size_t off, const UInt8 *b, size_t len);
size_t VpageObjRead(VpageObjRef vpageObjRef, size_t off, UInt8 *b, size_t len);

/* need to be replaced */
size_t VpageObjGetEndMarker(VpageObjRef vpageObjRef);
void VpageObjSetEndMarker(VpageObjRef vpageObjRef, size_t endMarker);

#endif /* EMEX64_VPAGEOBJ_H */
