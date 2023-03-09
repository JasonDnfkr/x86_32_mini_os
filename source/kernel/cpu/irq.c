#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "cpu/os_cfg.h"

#define IDT_TABLE_NO           128

static gate_desc_t idt_table[IDT_TABLE_NO];

void exception_handler_unknown(void);


static void do_default_handler(const char* message) {
    while (1) { }
}


void do_handler_unknown(void) {
    do_default_handler("unknown exception");
}


// 初始化中断
// 初始化IDT表
// lgdt 加载IDT表
void irq_init(void) {
    for (int i = 0; i < IDT_TABLE_NO; i++) {
        gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknown, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    }

    lidt((uint32_t)idt_table, sizeof(idt_table));
}