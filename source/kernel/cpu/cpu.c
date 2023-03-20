#include "cpu/cpu.h"
#include "cpu/os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"

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


void gate_desc_set(gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr) {
    desc->offset15_0 = offset & 0xffff;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xffff;
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


void swtch_to_tss(int tss_sel) {
    far_jump(tss_sel, 0);   // 起始地址，没有偏移
}


int gdt_alloc_desc(void) {
    irq_state_t state = irq_enter_protection();

    for (int i = 1; i < GDT_TABLE_SIZE; i++) {
        segment_desc_t* desc = gdt_table + i;
        if (desc->attr == 0) {

            irq_leave_protection(state);
            
            return i * sizeof(segment_desc_t);
        }
    }

    irq_leave_protection(state);

    return -1;
}