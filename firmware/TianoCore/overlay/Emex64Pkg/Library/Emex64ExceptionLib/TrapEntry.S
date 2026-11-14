/*
 * Copyright (C) 2026 emexlab
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#define CTX(i)      (16 + 8 * (i))
#define FR(i)       (296 + 8 * (i))

ASM_GLOBAL ASM_PFX(Emex64TrapEntry)

    .p2align 4
ASM_PFX(Emex64TrapEntry):
    stq [sp + 0], r0
    mov r0, sp
    sub r0, r0, 424
    and r0, r0, -16

    stq [r0 + CTX(1)], r1
    stq [r0 + CTX(2)], r2
    stq [r0 + CTX(3)], r3
    stq [r0 + CTX(4)], r4
    stq [r0 + CTX(5)], r5
    stq [r0 + CTX(6)], r6
    stq [r0 + CTX(7)], r7
    stq [r0 + CTX(8)], r8
    stq [r0 + CTX(9)], r9
    stq [r0 + CTX(10)], rr
    stq [r0 + CTX(11)], er0
    stq [r0 + CTX(12)], er1
    stq [r0 + CTX(13)], er2
    stq [r0 + CTX(14)], er3
    stq [r0 + CTX(15)], er4
    stq [r0 + CTX(16)], er5
    stq [r0 + CTX(17)], er6
    stq [r0 + CTX(18)], er7
    stq [r0 + CTX(19)], er8
    stq [r0 + CTX(20)], er9
    stq [r0 + CTX(21)], er10
    stq [r0 + CTX(22)], er11
    stq [r0 + CTX(23)], er12
    stq [r0 + CTX(24)], er13
    stq [r0 + CTX(25)], er14
    stq [r0 + CTX(26)], er15
    stq [r0 + CTX(27)], sp
    stq [r0 + CTX(28)], fp
    stq [r0 + CTX(29)], cf
    stq [r0 + CTX(30)], fpc
    ldq r1, [sp + 0]
    stq [r0 + CTX(0)], r1

    cmovb r1, 9
    stq [r0 + CTX(31)], r1
    cmovb r1, 7
    stq [r0 + CTX(32)], r1
    cmovb r1, 10
    stq [r0 + CTX(33)], r1
    cmovb r1, 11
    stq [r0 + CTX(34)], r1

    stq [r0 + FR(0)], fr0
    stq [r0 + FR(1)], fr1
    stq [r0 + FR(2)], fr2
    stq [r0 + FR(3)], fr3
    stq [r0 + FR(4)], fr4
    stq [r0 + FR(5)], fr5
    stq [r0 + FR(6)], fr6
    stq [r0 + FR(7)], fr7
    stq [r0 + FR(8)], fr8
    stq [r0 + FR(9)], fr9
    stq [r0 + FR(10)], fr10
    stq [r0 + FR(11)], fr11
    stq [r0 + FR(12)], fr12
    stq [r0 + FR(13)], fr13
    stq [r0 + FR(14)], fr14
    stq [r0 + FR(15)], fr15

    mov sp, r0
    mov r6, r0
    add r1, r0, 16
    ldq r0, [r1 + 8 * 33]
    bl ASM_PFX(Emex64TrapDispatch)

    ldq fr0, [r6 + FR(0)]
    ldq fr1, [r6 + FR(1)]
    ldq fr2, [r6 + FR(2)]
    ldq fr3, [r6 + FR(3)]
    ldq fr4, [r6 + FR(4)]
    ldq fr5, [r6 + FR(5)]
    ldq fr6, [r6 + FR(6)]
    ldq fr7, [r6 + FR(7)]
    ldq fr8, [r6 + FR(8)]
    ldq fr9, [r6 + FR(9)]
    ldq fr10, [r6 + FR(10)]
    ldq fr11, [r6 + FR(11)]
    ldq fr12, [r6 + FR(12)]
    ldq fr13, [r6 + FR(13)]
    ldq fr14, [r6 + FR(14)]
    ldq fr15, [r6 + FR(15)]

    add r1, r6, 16
    ldq r0, [r1 + 8 * 32]
    cmov 7, r0
    ldq r0, [r1 + 8 * 31]
    cmov 9, r0
    ldq fpc, [r1 + 8 * 30]
    ldq cf, [r1 + 8 * 29]

    ldq r0, [r1 + 8 * 0]
    ldq r2, [r1 + 8 * 2]
    ldq r3, [r1 + 8 * 3]
    ldq r4, [r1 + 8 * 4]
    ldq r5, [r1 + 8 * 5]
    ldq r6, [r1 + 8 * 6]
    ldq r7, [r1 + 8 * 7]
    ldq r8, [r1 + 8 * 8]
    ldq r9, [r1 + 8 * 9]
    ldq rr, [r1 + 8 * 10]
    ldq er0, [r1 + 8 * 11]
    ldq er1, [r1 + 8 * 12]
    ldq er2, [r1 + 8 * 13]
    ldq er3, [r1 + 8 * 14]
    ldq er4, [r1 + 8 * 15]
    ldq er5, [r1 + 8 * 16]
    ldq er6, [r1 + 8 * 17]
    ldq er7, [r1 + 8 * 18]
    ldq er8, [r1 + 8 * 19]
    ldq er9, [r1 + 8 * 20]
    ldq er10, [r1 + 8 * 21]
    ldq er11, [r1 + 8 * 22]
    ldq er12, [r1 + 8 * 23]
    ldq er13, [r1 + 8 * 24]
    ldq er14, [r1 + 8 * 25]
    ldq er15, [r1 + 8 * 26]
    ldq fp, [r1 + 8 * 28]
    ldq sp, [r1 + 8 * 27]
    ldq r1, [r1 + 8 * 1]
    eret
