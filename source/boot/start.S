/**
 * 系统引导部分，启动时由硬件加载运行，然后完成对二级引导程序loader的加载
 * 该部分程序存储于磁盘的第1个扇区，在计算机启动时将会由BIOS加载到0x7c00处
 * 之后，将由BIOS跳转至0x7c00处开始运行
 */
	#include "boot.h"

  	# 16位代码，务必加上
  	.code16
 	.text
	.global _start
	.extern boot_entry

_start:
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $_start, %esp

	# 显示字符 int 10，ah = 0xe, al = 字符ascii
	mov $0xe, %ah
	mov $'M', %al
	int $0x10
	mov $'I', %al
	int $0x10
	mov $'N', %al
	int $0x10
	mov $'I', %al
	int $0x10
	mov $'-', %al
	int $0x10
	mov $'O', %al
	int $0x10
	mov $'S', %al
	int $0x10

read_loader:
	mov $0x8000, %bx # 缓冲区地址
	mov $0x02,	 %ah 
	mov $0x02,	 %cx # CH 柱面， CL 扇区
	mov $64,	 %al # 64 = 32kb
	mov $0x80,	 %dx # DH 磁头，DL 驱动器: 80H 硬盘
	int $ 0x13
	jc read_loader   # 判断CF标志，如果为1就跳转

	jmp boot_entry

	jmp .
	
	# 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa
