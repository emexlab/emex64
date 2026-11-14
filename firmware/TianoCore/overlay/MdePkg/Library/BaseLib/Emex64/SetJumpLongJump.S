/*
 * Copyright (C) 2026 emexlab
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

ASM_GLOBAL ASM_PFX(SetJump)
ASM_GLOBAL ASM_PFX(InternalLongJump)

ASM_PFX(SetJump):
    stq [r0 + 0], r6
    stq [r0 + 8], r7
    stq [r0 + 16], r8
    stq [r0 + 24], r9
    stq [r0 + 32], er8
    stq [r0 + 40], er9
    stq [r0 + 48], er10
    stq [r0 + 56], er11
    stq [r0 + 64], er12
    stq [r0 + 72], er13
    stq [r0 + 80], er14
    stq [r0 + 88], er15
    add r1, fp, 16
    stq [r0 + 96], r1
    ldq r1, [fp + 8]
    stq [r0 + 104], r1
    ldq r1, [fp + 16]
    stq [r0 + 112], r1
    mov rr, 0
    ret

ASM_PFX(InternalLongJump):
    ldq r6, [r0 + 0]
    ldq r7, [r0 + 8]
    ldq r8, [r0 + 16]
    ldq r9, [r0 + 24]
    ldq er8, [r0 + 32]
    ldq er9, [r0 + 40]
    ldq er10, [r0 + 48]
    ldq er11, [r0 + 56]
    ldq er12, [r0 + 64]
    ldq er13, [r0 + 72]
    ldq er14, [r0 + 80]
    ldq er15, [r0 + 88]
    ldq er7, [r0 + 112]
    ldq fp, [r0 + 104]
    ldq sp, [r0 + 96]
    mov rr, r1
    b er7
