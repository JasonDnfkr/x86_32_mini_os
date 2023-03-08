#include "cpu/cpu.h"
#include "cpu/os_cfg.h"
#include "comm/cpu_instr.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];

// selector: 段选择子
// base:     基地址
// limit:    这段内存的大小
// attr:     属性值
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr) {
    segment_desc_t* desc = gdt_table + selector / sizeof(segment_desc_t);

    // 判断传入的 limit 是否大于 16 位
    // 若大于，则需要将单位转换为 KiB
    // limit 单位是 KiB 还是 B
    if (limit > 0xfffff) {
        attr |= SEG_G;     // 开启G位
        limit /= 0x1000;
    }
    
    desc->limit15_0 = limit & 0xffff;
    desc->base15_0  = base & 0xffff;
    desc->base23_16 = (base >> 16) & 0xff;
    desc->attr      = attr | (((limit >> 16) & 0xf) << 8);
    desc->base31_24 = (base >> 24) & 0xff;
}


void init_gdt(void) {
    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0);
    }
    
    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xffffffff, SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);

    // 代码段
    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xffffffff, SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}


void cpu_init(void) {
    init_gdt();
}