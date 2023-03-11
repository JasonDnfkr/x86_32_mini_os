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


.macro exception_handler name num with_err_code
    .text
    .global exception_handler_\name
    .extern do_handler_\name
# 中断处理函数
exception_handler_\name:
    # 首先CPU会自动压入一些寄存器到栈中(eflags, cs, ip)，可能会有 error code 也可能没有。
    # 没有的话，CPU不会压。所以为了方便（操作结构体），要自己手动给它压一个
    .if \with_err_code == 0
        push $0
    .endif

    # 中断号
    push $\num

    pusha # 该指令会保存一系列指定的寄存器
    push %ds
    push %es
    push %fs
    push %gs

    push %esp # 这里就是 struct exception_frame 的地址
    call do_handler_\name
    add $(1 * 4), %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2 * 4), %esp

    iret
.endm

exception_handler unknown,                      -1, 0
exception_handler divide_error,                  0, 0
exception_handler debug_exception,               1, 0
exception_handler nmi_interrupt,                 2, 0
exception_handler breakpoint,                    3, 0
exception_handler overflow,                      4, 0
exception_handler bound_range_exceed,            5, 0
exception_handler invalid_opcode,                6, 0
exception_handler device_not_found,              7, 0
exception_handler double_fault,                  8, 1
exception_handler invalid_tss,                   10, 1
exception_handler segment_not_present,           11, 1
exception_handler stack_segment_fault,           12, 0
exception_handler general_protection,            13, 1
exception_handler page_fault,                    14, 1
exception_handler floating_point_error,          16, 0
exception_handler aligment_check,                17, 1
exception_handler machine_check                  18, 0
exception_handler simd_floating_point_exception, 19, 0
exception_handler virtualization_exception,      20, 0
exception_handler control_protection_exception,  21, 1

exception_handler timer,  0x20, 0
