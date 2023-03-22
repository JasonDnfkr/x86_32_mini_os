	.text
	.global first_task_entry
	.extern first_task_main

    # ds, cs 寄存器会在进程创建前设置好
first_task_entry:
	mov %ss, %ax
	mov %ax, %ds
	# mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

    jmp first_task_main