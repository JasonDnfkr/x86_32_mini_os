#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/timer.h"
#include "tools/log.h"
#include "cpu/os_cfg.h"

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

    log_init();

    irq_init();
    timer_init();
}


void init_main(void) {
    // int a = 3 / 0;
    // irq_enable_global();
    log_printf("Kernel is running ...");
    log_printf("Version: %s %s", OS_VERSION, "test");
    log_printf("%d %d %x %c", 12345, -123, 0x123456, 'a');

    while (1) {  }
}