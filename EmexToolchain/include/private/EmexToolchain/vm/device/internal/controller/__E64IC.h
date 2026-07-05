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

#ifndef __E64IC_H
#define __E64IC_H

#include <EmexFoundation/EmexFoundation.h>

typedef struct __E64IC {
    EFObject header;
    UInt64 pending;
    UInt64 enabled;
    UInt64 ctrl;
    UInt64 vector_base;
    SInt64 current_irq;
} *__E64IC;

typedef struct __E64IC *E64ICRef;
typedef struct __E64Core *E64CoreRef;

Boolean __E64ServeInterruptIfNeeded(E64ICRef icRef, E64CoreRef coreRef);

#endif /* __E64IC_H */
