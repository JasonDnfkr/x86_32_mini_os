# NOW IN PROTECT MODE

    .text
    .extern kernel_init
    .global _start
_start:
    call kernel_init
    jmp .