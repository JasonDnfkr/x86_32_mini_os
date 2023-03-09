#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"

// void kernel_init() {
//     // while (1) { }
//     __asm__ __volatile__("nop");

// }

// int test_data = 4;
// const int test_rodata = 4;
// static int test_bss;

void kernel_init(boot_info_t* boot_info) {
    // while (1) { }
    __asm__ __volatile__("nop");

    cpu_init();
    irq_init();
}


void init_main(void) {
    int a = 3 / 0;
    while (1) {  }
}