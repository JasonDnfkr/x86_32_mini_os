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
    # 因为 KERNEL_SELECTOR_CS 为 1 * 8，所以会直接跳转到 gdt表的 CS所在的项，
    # 那里设置的地址范围是 0~4G，因此实际读取的地址就是 0 + gdt_reload 地址
    jmp $KERNEL_SELECTOR_CS, $gdt_reload

# 将段寄存器全部设置为 DS 指向的内容
gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    # 这里改了栈指针的位置，不再是7c00了
    # mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss # 定义一个 bss 区域
    # 定义一个内核初始化过程中使用的栈
    # 而不是去使用 loader 定义的栈（使用 loader 的栈0x7c00也行）
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
    call do_handler_\name # 这里调用了中断处理函数(trap_handler)，把上面压栈的内容正好当做参数
    add $(1 * 4), %esp  # 把上面那个 struct frame 跳过去

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2 * 4), %esp # 跳过压入的异常号和错误码

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


    .text
    .global simple_switch
simple_switch:  # simple_switch(&from->stack, to->stack)
    mov 4(%esp), %eax   # 取出 from 的 stack 字段
    mov 8(%esp), %edx   # 取出 to 的 stack 字段

    push %ebp
    push %ebx
    push %esi
    push %edi

    mov %esp, (%eax)   # 将现在的 esp 写入 from 的 stack 字段

    # 这里要从另外一个任务出栈, 切换栈
    # 把另一个任务的栈地址给了esp
    mov %edx, %esp

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    ret

    .global exception_handler_syscall
    .extern do_handler_syscall
exception_handler_syscall:
    pusha # 该指令会保存一系列指定的寄存器

    push %ds
    push %es
    push %fs
    push %gs

    pushf # push %eflags
    
	# 使用内核段寄存器，避免使用应用层的
	mov $(KERNEL_SELECTOR_DS), %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs    

    mov %esp, %eax  # 将这里作为结构体入口地址参数
    push %eax

    call do_handler_syscall

    add $4, %esp    # esp 向上调整，跳过结构体地址参数

    popf  # pop %eflags
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    retf $(5 * 4)
