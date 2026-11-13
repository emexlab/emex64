    .text
    .globl setjmp
setjmp:
    ldq r1, [fp + 8]
    stq r0, r1
    ldq r1, [fp + 16]
    stq [r0 + 8], r1
    add r1, fp, 16
    stq [r0 + 16], r1
    stq [r0 + 24], r6
    stq [r0 + 32], r7
    stq [r0 + 40], r8
    stq [r0 + 48], r9
    stq [r0 + 56], er8
    stq [r0 + 64], er9
    stq [r0 + 72], er10
    stq [r0 + 80], er11
    stq [r0 + 88], er12
    stq [r0 + 96], er13
    stq [r0 + 104], er14
    stq [r0 + 112], er15
#ifdef __emex64_fpu__
    stq [r0 + 120], fr8
    stq [r0 + 128], fr9
    stq [r0 + 136], fr10
    stq [r0 + 144], fr11
    stq [r0 + 152], fr12
    stq [r0 + 160], fr13
    stq [r0 + 168], fr14
    stq [r0 + 176], fr15
#endif
    mov rr, 0
    ret

    .globl longjmp
longjmp:
    mov rr, r1
    bnz rr, 1f
    mov rr, 1
1:
    ldq r6, [r0 + 24]
    ldq r7, [r0 + 32]
    ldq r8, [r0 + 40]
    ldq r9, [r0 + 48]
    ldq er8, [r0 + 56]
    ldq er9, [r0 + 64]
    ldq er10, [r0 + 72]
    ldq er11, [r0 + 80]
    ldq er12, [r0 + 88]
    ldq er13, [r0 + 96]
    ldq er14, [r0 + 104]
    ldq er15, [r0 + 112]
#ifdef __emex64_fpu__
    ldq fr8, [r0 + 120]
    ldq fr9, [r0 + 128]
    ldq fr10, [r0 + 136]
    ldq fr11, [r0 + 144]
    ldq fr12, [r0 + 152]
    ldq fr13, [r0 + 160]
    ldq fr14, [r0 + 168]
    ldq fr15, [r0 + 176]
#endif
    ldq r1, [r0 + 8]
    ldq sp, [r0 + 16]
    ldq fp, r0
    b r1
