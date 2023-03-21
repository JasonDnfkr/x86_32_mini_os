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
#include "tools/list.h"
#include "ipc/sem.h"
#include "core/memory.h"


void kernel_init(boot_info_t* boot_info) {
    // while (1) { }
    ASSERT(boot_info->ram_region_count != 0);
    // ASSERT(3 < 2);
    __asm__ __volatile__("nop");


    log_init();

    memory_init(boot_info);

    cpu_init();


    irq_init();
    timer_init();

    task_manager_init();
}

static task_t first_task;

// static task_t init_task;
// static uint32_t init_task_stack[1024];

// static sem_t sem;

// void init_task_entry(void) {
//     int count = 0;
//     while (1) {
//         log_printf("int task: %d", count++);

//         sem_wait(&sem);
//         // task_switch_from_to(&init_task, task_first_task());
//         // sys_sched_yield();
//         sys_sleep(1000);
//     }
// }


void link_test(void) {
    list_t list;
    list_node_t nodes[5];


    list_init(&list);
    log_printf("list: first = %x, last = %x, count = %d\n", list_first(&list), list_last(&list), list_size(&list));

    for (int i = 0; i < 5; i++) {
        list_node_t* node = &nodes[i];

        log_printf("%d, %x\n", i, (uint32_t)node);
        list_insert_front(&list, node);
    }

    for (int i = 0; i < 5; i++) {
        list_remove_front(&list);
        log_printf("[%d]", list_size(&list));
    }

    list_init(&list);

    for (int i = 0; i < 5; i++) {
        list_node_t* node = &nodes[i];

        log_printf("%d, %x\n", i, (uint32_t)node);
        list_insert_back(&list, node);
    }

    struct type_t {
        int i;
        list_node_t node;
    } v = { 0x123456 };

    list_node_t* v_node = &v.node;
    struct type_t* p = list_node_parent(v_node, struct type_t, node);

    if (p->i != 0x123456) {
        log_printf("error!");
    }

}


void move_to_first_task(void) {
    void first_task_entry(void);
    task_t* curr = task_current();

    ASSERT(curr != 0);

    tss_t* tss = &(curr->tss);

    // 也可以使用类似boot跳loader中的函数指针跳转
    // 这里用jmp是因为后续需要使用内联汇编添加其它代码
    __asm__ __volatile__(
        // 模拟中断返回，切换入第1个可运行应用进程
        // 不过这里并不直接进入到进程的入口，而是先设置好段寄存器，再跳过去
        "push %[ss]\n\t"			// SS
        "push %[esp]\n\t"			// ESP
        "push %[eflags]\n\t"           // EFLAGS
        "push %[cs]\n\t"			// CS
        "push %[eip]\n\t"		    // ip
        "iret\n\t"::[ss]"r"(tss->ss),  [esp]"r"(tss->esp), [eflags]"r"(tss->eflags),
        [cs]"r"(tss->cs), [eip]"r"(tss->eip)
    );

    // far_jump((uint32_t)tss, 0);
}


void init_main(void) {
    // int a = 3 / 0;
    // irq_enable_global();
    log_printf("Kernel is running ...");
    log_printf("Version: %s %s", OS_VERSION, "test");
    log_printf("%d %d %x %c", 12345, -123, 0x123456, 'a');

    //
    // task_init(&init_task, "init task", (uint32_t)init_task_entry, (uint32_t)&init_task_stack[1024]);
    //

    task_first_init();
    move_to_first_task();

    /*

    int count = 0;

    // link_test();

    irq_enable_global();

    while (1) {
        log_printf("int main: %d", count++);
        // task_switch_from_to(task_first_task(), &init_task);
        // sys_sched_yield();
        // sem_notify(&sem);
        sys_sleep(1000);
    }

    */
}