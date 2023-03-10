#ifndef IRQ_H
#define IRQ_H

#include <comm/types.h>

#define IRQ0_DIVIDE_ERROR       0

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

void exception_handler_divide_error(void);

int irq_install(int irq_num, irq_handler_t handler);


// // 默认缺省中断
// void do_handler_unknown(exception_frame_t* frame);

// // divide by zero
// void do_handler_divide_error(exception_frame_t* frame);

#endif