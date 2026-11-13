    .text
    .globl memcpy
memcpy:
    cpym r0, r1, r2
    mov rr, r0
    ret

    .globl memmove
memmove:
    movm r0, r1, r2
    mov rr, r0
    ret

    .globl memset
memset:
    setm r0, r1, r2
    mov rr, r0
    ret
