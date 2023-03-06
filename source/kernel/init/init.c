#include "init.h"
#include "comm/boot_info.h"

// void kernel_init() {
//     // while (1) { }
//     __asm__ __volatile__("nop");

// }

void kernel_init(boot_info_t* boot_info) {
    // while (1) { }
    __asm__ __volatile__("nop");

}