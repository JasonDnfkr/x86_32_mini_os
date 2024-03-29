#include "cpu/irq.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "cpu/os_cfg.h"
#include "tools/log.h"

#define IDT_TABLE_NO           128

static gate_desc_t idt_table[IDT_TABLE_NO];


static void dump_core_regs(exception_frame_t* frame) {
    // 判断发生异常时的特权级，取CS寄存器的标志位可得
    uint32_t esp;
    uint32_t ss;
    if (frame->cs & 0x7) {
        ss = frame->ds;
        esp = frame->esp;
    } else {
        ss = frame->ss3;
        esp = frame->esp3;
    }

    log_printf("IRQ: %d, Error Code: %d", frame->num, frame->error_code);
    log_printf("cs: %d\nds: %d\nes: %d\nss: %d\nfs: %d\ngs: %d\n", 
               frame->cs, frame->ds, frame->es, ss, frame->fs, frame->gs);
    log_printf("eax: %x\nebx: %x\nedx: %x\nedi: %x\nesi: %x\nebp: %x\nesp: %x\n", 
               frame->eax, frame->ebx, frame->edx, frame->edi, frame->esi, frame->ebp, esp);
    log_printf("eip: %x\neflags: %x\n", frame->eip, frame->eflags);
}


static void do_default_handler(exception_frame_t* frame, const char* message) {
    log_printf("------------------------------------------");
    log_printf("IRQ/Exception: %s", message);
    dump_core_regs(frame);

    while (1) {
        hlt();
    }
}

// 默认缺省中断
void do_handler_unknown(exception_frame_t* frame) {
    do_default_handler(frame, "unknown exception");
}


// Exception: divide by zero
void do_handler_divide_error(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Divide by zero");
}


// Exception: 
void do_handler_debug_exception(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Debug exception");
}   


// Exception: 
void do_handler_nmi_interrupt(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: NMI interrupt");
}   


// Exception: 
void do_handler_breakpoint(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Breakpoint");
}   


// Exception: 
void do_handler_overflow(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Overflow");
}   


// Exception: 
void do_handler_bound_range_exceed(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Bound range exceed");
}   


// Exception: 
void do_handler_invalid_opcode(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Invalid opcode");
}   


// Exception: 
void do_handler_device_not_found(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Device not found");
}   


// Exception: 
void do_handler_double_fault(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Double fault");
}   


// Exception: 
void do_handler_invalid_tss(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Invalid tss");
}   


// Exception: 
void do_handler_segment_not_present(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Segment not present");
}   


// Exception: 
void do_handler_stack_segment_fault(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Stack segment fault");
}   


// Exception: 
void do_handler_general_protection(exception_frame_t* frame) {
    log_printf("--------------------------------");
    log_printf("IRQ/Exception happend: General Protection.");
    if (frame->error_code & ERR_EXT) {
        log_printf("the exception occurred during delivery of an "
                "event external to the program, such as an interrupt"
                "or an earlier exception.");
    } 
    else {
        log_printf("the exception occurred during delivery of a"
                    "software interrupt (INT n, INT3, or INTO).");
    }
    
    if (frame->error_code & ERR_IDT) {
        log_printf("the index portion of the error code refers "
                    "to a gate descriptor in the IDT");
    } 
    else {
        log_printf("the index refers to a descriptor in the GDT");
    }
    
    log_printf("segment index: %d", frame->error_code & 0xFFF8);

    dump_core_regs(frame);
    while (1) {
        hlt();
    }	
}   


// Exception: Page Fault
void do_handler_page_fault(exception_frame_t* frame) {
    log_printf("--------------------------------");
    log_printf("IRQ/Exception: Page fault.");
    // 是否页不存在
    // The fault was caused by a non-present page.
    if (frame->error_code & ERR_PAGE_P) {
        log_printf("\tpage-level protection violation: %x.", read_cr2());
    } 
    else {
        log_printf("\tThe fault was caused by a non-present page. %x", read_cr2());
    }
    
    if (frame->error_code & ERR_PAGE_WR) {
        log_printf("\tThe access causing the fault was a read.");
    } 
    else {
        log_printf("\tThe access causing the fault was a write.");
    }
    
    if (frame->error_code & ERR_PAGE_US) {
        log_printf("\tA supervisor-mode access caused the fault.");
    } 
    else {
        log_printf("\tA user-mode access caused the fault.");
    }

    dump_core_regs(frame);
    while (1) {
        hlt();
    }
}   


// Exception: 
void do_handler_floating_point_error(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Floating point error");
}   


// Exception: 
void do_handler_aligment_check(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Aligment check");
}   


// Exception: 
void do_handler_machine_check(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Machine check");
}   


// Exception: 
void do_handler_simd_floating_point_exception(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: SIMD floating point exception");
}


// Exception: 
void do_handler_virtualization_exception(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Virtualization exception");
}


// Exception: 
void do_handler_control_protection_exception(exception_frame_t* frame) {
    do_default_handler(frame, "Exception: Control protection exception");
}


// 初始化 8259 中断芯片
static void init_pic(void) {
    // 对第一个 8259 芯片初始化
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC0_ICW2, IRQ_PIC_START);
    outb(PIC0_ICW3, 1 << 2);
    outb(PIC0_ICW4, PIC_ICW4_8086);

    // 对第二个 8259 芯片初始化
    outb(PIC1_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC1_ICW2, IRQ_PIC_START + 8);
    outb(PIC1_ICW3, 2);
    outb(PIC1_ICW4, PIC_ICW4_8086);

    // 此时还没有给外部设备设置好寄存器，（还没初始化好外部设备）
    // 所以此时应该屏蔽所有的中断
    outb(PIC0_IMR, 0xff & ~(1 << 2));
    outb(PIC1_IMR, 0xff);
}


// 初始化中断
// 初始化IDT表
// lgdt 加载IDT表
void irq_init(void) {
    for (int i = 0; i < IDT_TABLE_NO; i++) {
        gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t)exception_handler_unknown, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
    }

    irq_install(IRQ0_DE_DIVIDE_ERROR, (irq_handler_t)exception_handler_divide_error);
    irq_install(IRQ1_DB_DEBUG_EXCEPTION, (irq_handler_t)exception_handler_debug_exception);
    irq_install(IRQ2_NMI_INTERRUPT, (irq_handler_t)exception_handler_nmi_interrupt);
    irq_install(IRQ3_BP_BREAKPOINT, (irq_handler_t)exception_handler_breakpoint);
    irq_install(IRQ4_OF_OVERFLOW, (irq_handler_t)exception_handler_overflow);
    irq_install(IRQ5_BR_BOUND_RANGE_EXCEEDED, (irq_handler_t)exception_handler_bound_range_exceed);
    irq_install(IRQ6_UD_INVALID_OPCODE, (irq_handler_t)exception_handler_invalid_opcode);
    irq_install(IRQ7_NM_DEVICE_NOT_AVAILABLE, (irq_handler_t)exception_handler_device_not_found);
    irq_install(IRQ8_DF_DOUBLE_FAULT, (irq_handler_t)exception_handler_double_fault);
    irq_install(IRQ10_TS_INVALID_TSS, (irq_handler_t)exception_handler_invalid_tss);
    irq_install(IRQ11_NP_SEGMENT_NOT_PRESENT, (irq_handler_t)exception_handler_segment_not_present);
    irq_install(IRQ12_SS_STACK_SEGMENT_FAULT, (irq_handler_t)exception_handler_stack_segment_fault);
    irq_install(IRQ13_GP_GENERAL_PROTECTION, (irq_handler_t)exception_handler_general_protection);
    irq_install(IRQ14_PF_PAGE_FAULT, (irq_handler_t)exception_handler_page_fault);
    irq_install(IRQ16_MF_FLOATING_POINT_ERR, (irq_handler_t)exception_handler_floating_point_error);
    irq_install(IRQ17_AC_ALIGNMENT_CHECK, (irq_handler_t)exception_handler_aligment_check);
    irq_install(IRQ18_MC_MACHINE_CHECK, (irq_handler_t)exception_handler_machine_check);
    irq_install(IRQ19_XM_SIMD_FLOATING_POINT_EXCEPTION, (irq_handler_t)exception_handler_simd_floating_point_exception);
    irq_install(IRQ20_VE_VITUALIZATION_EXCEPTION, (irq_handler_t)exception_handler_virtualization_exception);
    irq_install(IRQ21_CP_CONTROL_PROTECTION_EXCEPITON, (irq_handler_t)exception_handler_control_protection_exception);

    lidt((uint32_t)idt_table, sizeof(idt_table));

    init_pic();
}


// 将中断编号加载至IDT表
int irq_install(int irq_num, irq_handler_t handler) {
    if (irq_num >= IDT_TABLE_NO) {
        return -1;
    }

    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_INT);
}

// 开启 8259 特定的中断
void irq_enable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START; // 将值转换为 8259 对应的从0开始的内部值
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num); // 清0
        outb(PIC0_IMR, mask);   // 回写
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num); // 清0
        outb(PIC1_IMR, mask);   // 回写
    }
}

// 关闭 8259 特定的中断
void irq_disable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START; // 将值转换为 8259 对应的从0开始的内部值
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) | (1 << irq_num); // 清0
        outb(PIC0_IMR, mask);   // 回写
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) | (1 << irq_num); // 清0
        outb(PIC1_IMR, mask);   // 回写
    }
}


// 关闭全局中断
void irq_disable_global(void) {
    cli();
}

// 开启全局中断
void irq_enable_global(void) {
    sti();
}

// 通知定时器继续响应下一次中断
void pic_send_eoi(int irq_num) {
    irq_num -= IRQ_PIC_START;
    if (irq_num >= 8) {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }

    outb(PIC0_OCW2, PIC_OCW2_EOI);
}

// 进入临界区保护状态
irq_state_t irq_enter_protection(void) {
    irq_state_t state = read_eflags();
    irq_disable_global();
    return state;
}

// 退出临界区保护状态
irq_state_t irq_leave_protection(irq_state_t state) {
    write_eflags(state);
}