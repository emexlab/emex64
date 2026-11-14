/** @file
  Copyright (C) 2026 emexlab
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#define MDE_CPU_EMEX64

#if !defined(__GNUC__)
  #pragma pack()
#endif /* !defined(__GNUC__) */

typedef unsigned long long UINT64 __attribute__ ((aligned (8)));
typedef long long INT64 __attribute__ ((aligned (8)));

typedef unsigned int UINT32 __attribute__ ((aligned (4)));
typedef int INT32 __attribute__ ((aligned (4)));

typedef unsigned short UINT16 __attribute__ ((aligned (2)));

typedef unsigned short CHAR16 __attribute__ ((aligned (2)));

typedef short INT16 __attribute__ ((aligned (2)));

typedef unsigned char BOOLEAN;
typedef unsigned char UINT8;
typedef char CHAR8;
typedef signed char INT8;

typedef UINT64 UINTN __attribute__ ((aligned (8)));
typedef INT64 INTN __attribute__ ((aligned (8)));

#define MAX_BIT                             0x8000000000000000ULL
#define MAX_2_BITS                          0xC000000000000000ULL
#define MAX_ADDRESS                         0xFFFFFFFFFFFFFFFFULL
#define MAX_ALLOC_ADDRESS                   0x1FFFFFFFFFFFFFULL
#define MAX_INTN                            ((INTN)0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINTN                           ((UINTN)0xFFFFFFFFFFFFFFFFULL)
#define MIN_INTN                            (((INTN)-9223372036854775807LL) - 1)
#define CPU_STACK_ALIGNMENT                 16
#define DEFAULT_PAGE_ALLOCATION_GRANULARITY (0x1000)
#define RUNTIME_PAGE_ALLOCATION_GRANULARITY (0x2000)

#ifndef EFIAPI
#define EFIAPI
#endif /* EFIAPI */

#if defined(__GNUC__)
#define ASM_GLOBAL  .globl
#endif /* defined(__GNUC__) */

#define FUNCTION_ENTRY_POINT(FunctionPointer)   (VOID *)(UINTN)(FunctionPointer)

#ifndef __USER_LABEL_PREFIX__
#define __USER_LABEL_PREFIX__
#endif /* __USER_LABEL_PREFIX__ */
