# NOW IN PROTECT MODE
#include "cpu/os_cfg.h"

    .text
    .extern kernel_init
    .global _start
    .extern init_main
_start:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax # 取传入的参数 boot_info_t* boot_info

    # mov 4(%esp), %eax # 这条语句 可以代替上面三条指令。没有按照gcc的C语言函数调用约定来做事，因为_start不会返回

    push %eax # 手动压入下一个要执行的函数的参数，相当于 kernel_init(&boot_info)
    # kernel_init(&boof_info);
    call kernel_init

    # jmp 语法格式：偏移量, 地址
    # 因为 KERNEL_SELECTOR_CS 为 0，所以会直接跳转到 gdt_reload 地址处
    jmp $KERNEL_SELECTOR_CS, $gdt_reload

# 将段寄存器全部设置为 DS 指向的内容
gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss # 定义一个 bss 区域
    # 定义一个内核初始化过程中使用的栈
    # 而不是去使用 loader 定义的栈（使用 loader 的栈也行）
    .comm stack, KERNEL_STACK_SIZE 