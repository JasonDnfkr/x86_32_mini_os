#ifndef CPU_H
#define CPU_H

#include "comm/types.h"

#pragma pack(1)
typedef struct _segment_desc_t {
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t  base23_16;
    uint16_t attr;
    uint8_t  base31_24;
} segment_desc_t;

#pragma pack()


// segment descriptor: G位
#define SEG_G           (1 << 15)

// segment descriptor: DPL位，控制某些代码段是16位还是32位。一般都需要设置此项
#define SEG_D           (1 << 14)

// segment descriptor: P位，指示当前的段描述符是否存在
#define SEG_P_PRESENT   (1 << 7)

// segment descriptor: DPL位，高特权级位
#define SEG_DPL0        (0 << 5)

// segment descriptor: DPL位，低特权级位
#define SEG_DPL3        (3 << 5)

// segment descriptor: S位，系统段
#define SEG_S_SYSTEM    (0 << 4)

// segment descriptor: S位，普通数据段
#define SEG_S_NORMAL    (1 << 4)


// 初始化 CPU 相关的数据结构
void cpu_init(void);


// 设置GDT表项
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);



#endif