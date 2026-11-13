#ifndef EMEX64_CRT_STACK_SIZE
#define EMEX64_CRT_STACK_SIZE 0x100000
#endif  ; EMEX64_CRT_STACK_SIZE

    .section .bss
    .p2align 3
__emex64_crt_stack:
    .space EMEX64_CRT_STACK_SIZE
    .globl __emex64_boot_arg
__emex64_boot_arg:
    .space 8

    .section .text.start, "ax", @progbits
    .globl _start
_start:
    .cfi_startproc
    .cfi_undefined pc
    mov r6, r0
    mov r0, __bss_start
    mov r1, __bss_end
    sub r1, r1, r0
    setm r0, 0, r1
    stq __emex64_boot_arg, r6
    mov sp, __emex64_crt_stack + EMEX64_CRT_STACK_SIZE - 8
    mov fp, sp
    mov tp, __tls_block
    mov r1, __tdata_start
    mov r2, __tdata_end
    sub r2, r2, r1
    cpym tp, r1, r2
    mov r6, __init_array_start
2:
    cmp r6, __init_array_end
    bge 3f
    ldq r0, r6
    bl r0
    add r6, r6, 8
    b 2b
3:
    mov r0, 0
    mov r1, 0
    bl main
    mov r0, rr
    bl exit
1:
    hlt
    b 1b
    .cfi_endproc
