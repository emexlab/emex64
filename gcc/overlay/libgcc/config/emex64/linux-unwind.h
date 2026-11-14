/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 emexlab
 */

#ifndef EMEX64_LINUX_UNWIND_H
#define EMEX64_LINUX_UNWIND_H

#ifdef __emex64_pauth__
static inline void *emex64_strip_pac(_Unwind_Word addr)
{
    _Unwind_Word r;
    __asm__("xpac\t%0, %1" : "=r" (r) : "r" (addr));
    return (void *)r;
}

#define MD_DEMANGLE_RETURN_ADDR(context, fs, addr) emex64_strip_pac(addr)
#endif /* __emex64_pauth__ */

#endif /* EMEX64_LINUX_UNWIND_H */
