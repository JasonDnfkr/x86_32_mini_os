#ifndef CPU_H
#define CPU_H

#include "comm/types.h"

#define EFLAGS_DEFAULT          (1 << 1)
#define EFLAGS_IF               (1 << 9)


#pragma pack(1)

// GDT 表项
typedef struct _segment_desc_t {
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t  base23_16;
    uint16_t attr;
    uint8_t  base31_24;
} segment_desc_t;


// Interrupt Gate
typedef struct _gate_desc_t {
    uint16_t offset15_0;
    uint16_t selector;
    uint16_t attr;
    uint16_t offset31_16;
} gate_desc_t;

#pragma pack()


// segment descriptor: G位，指示该内存大于 16 位 (64 KiB)
#define SEG_G           (1 << 15)

// segment descriptor: DPL位，设置该段是32位代码
// 控制某些代码段是16位还是32位。一般都需要设置此项
#define SEG_D           (1 << 14)

// segment descriptor: P位，指示当前的段描述符是否存在
#define SEG_P_PRESENT   (1 << 7)

// segment descriptor: DPL位，高特权级位
#define SEG_DPL0        (0 << 5)

// segment descriptor: DPL位，低特权级位
#define SEG_DPL3        (3 << 5)

// segment descriptor: CPL位，高特权级位
#define SEG_CPL0        (0 << 0)

// segment descriptor: CPL位，低特权级位
#define SEG_CPL3        (3 << 0)

#define SEG_RPL0                (0 << 0)
#define SEG_RPL3                (3 << 0)

// segment descriptor: S位，系统段 （中断，TSS）
#define SEG_S_SYSTEM    (0 << 4)

// segment descriptor: S位，普通段 （代码，数据）
#define SEG_S_NORMAL    (1 << 4)

// segment descriptor: Type位，代码段
#define SEG_TYPE_CODE   (1 << 3)

// segment descriptor: Type位，数据段
#define SEG_TYPE_DATA   (0 << 3)

// segment descriptor: Type位，TSS段
#define SEG_TYPE_TSS    (9 << 0)

// segment descriptor: Type位
// 对于数据段，置1指示该段可读写
// 对于代码段，置1指示该段代码可读取可执行，置0指示该段代码只能执行
#define SEG_TYPE_RW     (1 << 1)

// interrupt gate: P
#define GATE_P_PRESENT  (1 << 15)

// interrupt gate: DPL0
#define GATE_DPL0       (0 << 13)

// interrupt gate: DPL3
#define GATE_DPL3       (3 << 13)

// interrupt gate: 
#define GATE_TYPE_INT   (0xE << 8)


// TSS
typedef struct _tss_t {
    uint32_t pre_link;
    
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;

    uint32_t ldt;
    uint32_t iomap;
} tss_t;



// 初始化 CPU 相关的数据结构
void cpu_init(void);


// 设置GDT表项
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);


// 设置 Interrupt Gate 表项
void gate_desc_set(gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr);

// 从GDT表中找一个空闲项，找到后返回该地址
int gdt_alloc_desc(void);

// 清空GDT表项
void gdt_free_sel(int sel);

// 跳转至tss
void swtch_to_tss(int tss_sel);

#endif