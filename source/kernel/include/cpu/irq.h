#ifndef IRQ_H
#define IRQ_H

#include <comm/types.h>

#define IRQ0_DE_DIVIDE_ERROR         0
#define IRQ1_DB_DEBUG_EXCEPTION      1
#define IRQ2_NMI_INTERRUPT           2
#define IRQ3_BP_BREAKPOINT           3
#define IRQ4_OF_OVERFLOW             4
#define IRQ5_BR_BOUND_RANGE_EXCEEDED 5
#define IRQ6_UD_INVALID_OPCODE       6
#define IRQ7_NM_DEVICE_NOT_AVAILABLE 7
#define IRQ8_DF_DOUBLE_FAULT         8
#define IRQ9_RESERVED                9
#define IRQ10_TS_INVALID_TSS                    10
#define IRQ11_NP_SEGMENT_NOT_PRESENT            11
#define IRQ12_SS_STACK_SEGMENT_FAULT            12
#define IRQ13_GP_GENERAL_PROTECTION             13
#define IRQ14_PF_PAGE_FAULT                     14
#define IRQ15_RESERVED                          15
#define IRQ16_MF_FLOATING_POINT_ERR             16
#define IRQ17_AC_ALIGNMENT_CHECK                17
#define IRQ18_MC_MACHINE_CHECK                  18
#define IRQ19_XM_SIMD_FLOATING_POINT_EXCEPTION  19
#define IRQ20_VE_VITUALIZATION_EXCEPTION        20
#define IRQ21_CP_CONTROL_PROTECTION_EXCEPITON   21


// 定时器中断
#define IRQ0_TIMER                              0x20

// 第1个8259
#define PIC0_ICW1       0x20        // 第1个寄存器
#define PIC0_ICW2       0x21        // 第2个寄存器
#define PIC0_ICW3       0x21        // 第3个寄存器
#define PIC0_ICW4       0x21        // 第4个寄存器
#define PIC0_IMR        0X21        // 中断屏蔽寄存器

#define PIC0_OCW2       0x20


// 第2个8259
#define PIC1_ICW1       0xa0        // 第1个寄存器
#define PIC1_ICW2       0xa1        // 第2个寄存器
#define PIC1_ICW3       0xa1        // 第3个寄存器
#define PIC1_ICW4       0xa1        // 第4个寄存器
#define PIC1_IMR        0xa1        // 中断屏蔽寄存器

#define PIC1_OCW2       0xa0

#define PIC_ICW1_ALWAYS_1           (1 << 4)
#define PIC_ICW1_ICW4               (1 << 0)
#define PIC_ICW4_8086               (1 << 0)

#define PIC_OCW2_EOI                (1 << 5)

#define IRQ_PIC_START               0x20


typedef struct _exception_frame_t {
    uint32_t gs;                    
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t num;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} exception_frame_t;


typedef void (*irq_handler_t)(exception_frame_t* frame);


void irq_init(void);

void exception_handler_unknown(void);
void exception_handler_divide_error(void);
void exception_handler_debug_exception(void);       
void exception_handler_nmi_interrupt(void);         
void exception_handler_breakpoint(void);            
void exception_handler_overflow(void);              
void exception_handler_bound_range_exceed(void);    
void exception_handler_invalid_opcode(void);        
void exception_handler_device_not_found(void);      
void exception_handler_double_fault(void);          
void exception_handler_invalid_tss(void);            
void exception_handler_segment_not_present(void);    
void exception_handler_stack_segment_fault(void);    
void exception_handler_general_protection(void);     
void exception_handler_page_fault(void);             
void exception_handler_floating_point_error(void);   
void exception_handler_aligment_check(void);         
void exception_handler_machine_check(void);          
void exception_handler_simd_floating_point_exception(void);
void exception_handler_virtualization_exception(void);
void exception_handler_control_protection_exception(void);


void irq_enable(int irq_num);
void irq_disable(int irq_num);
void irq_enable_global(void);
void irq_disable_global(void);


// 通知定时器继续响应下一次中断
void pic_send_eoi(int irq_num);


int irq_install(int irq_num, irq_handler_t handler);


// // 默认缺省中断
// void do_handler_unknown(exception_frame_t* frame);

// // divide by zero
// void do_handler_divide_error(exception_frame_t* frame);

#endif