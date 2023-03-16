#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/timer.h"
#include "tools/log.h"
#include "cpu/os_cfg.h"
#include "tools/klib.h"
#include "core/task.h"
#include "comm/cpu_instr.h"
// void kernel_init() {
//     // while (1) { }
//     __asm__ __volatile__("nop");

// }

// int test_data = 4;
// const int test_rodata = 4;
// static int test_bss;

void kernel_init(boot_info_t* boot_info) {
    // while (1) { }
    ASSERT(boot_info->ram_region_count != 0);
    // ASSERT(3 < 2);
    __asm__ __volatile__("nop");

    cpu_init();

    log_init();

    irq_init();
    timer_init();
}


static task_t init_task;
static uint32_t init_task_stack[1024];
static task_t first_task;


void init_task_entry(void) {
    int count = 0;
    while (1) {
        log_printf("int task: %d", count++);
        task_switch_from_to(&init_task, &first_task);
    }
}


void init_main(void) {
    // int a = 3 / 0;
    // irq_enable_global();
    log_printf("Kernel is running ...");
    log_printf("Version: %s %s", OS_VERSION, "test");
    log_printf("%d %d %x %c", 12345, -123, 0x123456, 'a');

    //
    task_init(&first_task, (uint32_t)0, 0);
    task_init(&init_task, (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]);

    // 初始化第一个任务的TR寄存器，表示当前运行的任务是tss_sel参数中指向的任务
    write_tr(first_task.tss_sel);
    //

    int count = 0;

    while (1) {
        log_printf("int main: %d", count++);
        task_switch_from_to(&first_task, &init_task);
    }
}