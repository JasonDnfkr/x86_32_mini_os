#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "cpu/os_cfg.h"

#define IDT_TABLE_NO           128

static gate_desc_t idt_table[IDT_TABLE_NO];

void exception_handler_unknown(void);


static void do_default_handler(exception_frame_t* frame, const char* message) {
    while (1) { }
}

// 默认缺省中断
void do_handler_unknown(exception_frame_t* frame) {
    do_default_handler(frame, "unknown exception");
}


// divide by zero
void do_handler_divide_error(exception_frame_t* frame) {
    do_default_handler(frame, "divide exception");
}


// 初始化中断
// 初始化IDT表
// lgdt 加载IDT表
void irq_init(void) {
    for (int i = 0; i < IDT_TABLE_NO; i++) {
        gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknown, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    }

    irq_install(IRQ0_DIVIDE_ERROR, (irq_handler_t)exception_handler_divide_error);

    lidt((uint32_t)idt_table, sizeof(idt_table));
}


// 将中断编号加载至IDT表
int irq_install(int irq_num, irq_handler_t handler) {
    if (irq_num >= IDT_TABLE_NO) {
        return -1;
    }

    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
}