# NOW IN PROTECT MODE

    .text
    .extern kernel_init
    .global _start
_start:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax # 取传入的参数 boot_info_t* boot_info

    # mov 4(%esp), %eax # 这条语句 可以代替上面三条指令。没有按照gcc的C语言函数调用约定来做事，因为_start不会返回

    push %eax # 手动压入下一个要执行的函数的参数，相当于 kernel_init(&boot_info)
    call kernel_init
    jmp .