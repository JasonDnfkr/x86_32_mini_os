#include "core/task.h"
#include "tools/klib.h"
#include "cpu/os_cfg.h"
#include "cpu/cpu.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "core/memory.h"
#include "cpu/mmu.h"

// 全局的进程队列
static task_manager_t task_manager;

// 空闲进程的栈, 不是广义上的内核栈
static uint32_t idle_task_stack[IDLE_TASK_SIZE];


static int tss_init(task_t* task, int flag, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        log_printf("alloc tss failed\n");
        return -1;
    } 

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS);

    kmemset(&task->tss, 0, sizeof(tss_t));

    // 分配一个内核栈空间，大小为PGSIZE
    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0) {
        goto tss_init_failed;
    }

    int code_sel;
    int data_sel;
    if (flag & TASK_FLAGS_SYSTEM) {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    }
    else {
        code_sel = task_manager.app_code_sel | SEG_RPL3;
        data_sel = task_manager.app_data_sel | SEG_RPL3;
    }

    task->tss.eip    = entry;
    task->tss.esp    = esp;
    task->tss.esp0   = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss     = data_sel;
    task->tss.ss0    = KERNEL_SELECTOR_DS; 
    task->tss.es     = task->tss.ds  = task->tss.fs = task->tss.gs = data_sel;
    task->tss.cs     = code_sel;
    task->tss.eflags = EFLAGS_IF | EFLAGS_DEFAULT;

    // CR3 页表
    uint32_t uvm_pgtbl = memory_create_uvm();
    if (uvm_pgtbl == 0) {
        goto tss_init_failed;
    }
    task->tss.cr3    = uvm_pgtbl;

    task->tss_sel    = tss_sel;
    return 0;

// 资源回收
tss_init_failed:
    gdt_free_sel(tss_sel);
    if (kernel_stack != 0) {
        memory_free_page(kernel_stack);
    }
    return -1;    
}

// 创建一个进程。
// task_t* task:     已申请内存的进程结构体
// const char* name: 进程名
// int flag:         TASK_FLAG_USER 为用户进程，TASK_FLAG_SYSTEM 为内核进程
// uint32_t entry:   进程入口地址
// uint32_t esp:     进程栈底指针
int task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp) {
    ASSERT(task != (task_t*)0);

    tss_init(task, flag, entry, esp);

    kstrncpy(task->name, name, TASK_NAME_SIZE);
    task->state = TASK_CREATED;

    task->sleep_ticks = 0;
    task->time_ticks = TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks = task->time_ticks;

    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    irq_state_t state = irq_enter_protection();


    task->pid = (uint32_t)task;

    task_set_ready(task);

    list_insert_back(&task_manager.task_list, &task->all_node);

    irq_leave_protection(state);

    return 0;
}


void simple_switch(uint32_t** from, uint32_t* to);


void task_switch_from_to(task_t* from, task_t* to) {
    swtch_to_tss(to->tss_sel);
    // simple_switch(&from->stack, to->stack);
}


// 空闲进程的执行代码
static void idle_task_entry(void) {
    while (1) {
        hlt();
    }
}


void task_manager_init(void) {
    //数据段和代码段，使用DPL3，所有应用共用同一个
    //为调试方便，暂时使用DPL0
    int sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                     SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);
    task_manager.app_data_sel = sel;

    sel = gdt_alloc_desc();
    segment_desc_set(sel, 0x00000000, 0xFFFFFFFF,
                     SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |
                     SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);
    task_manager.app_code_sel = sel;

    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t*)0;

    // 创建一个 idle 进程，当所有进程都在sleep时，调用它
    task_init(&task_manager.idle_task, 
              "idle_task", 
              TASK_FLAGS_SYSTEM,
              (uint32_t)idle_task_entry, 
              (uint32_t)idle_task_stack + IDLE_TASK_SIZE
    );

    task_manager.curr_task = &task_manager.idle_task;
}

// 初始化程序的第一个进程，是操作系统一直连贯的
// 等切换走的时候，会自动保存进去的
void task_first_init(void) {
    void first_task_entry(void);
    extern uint8_t s_first_task[], e_first_task[];

    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start = (uint32_t)first_task_entry;

    /* first_task + alloc_size 是分配的地址的末尾，作为栈底*/
    task_init(&task_manager.first_task, "first task", TASK_FLAGS_USER, (uint32_t)first_start, first_start + alloc_size);

    // 初始化第一个任务的TR寄存器，表示当前运行的任务是tss_sel参数中指向的任务
    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task = &task_manager.first_task;

    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    // 给该进程新开个一级页表，也就是类似于用户页表的东西
    // 并在这个一级页表里申请 alloc_size 大小的内存
    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);

    // 把这段代码拷贝到上面生成的页表里
    // 相当于就是把内存搬运到 virtual 0x80000000+ 了
    kmemcpy((void*)first_start, s_first_task, copy_size);
}

// 获取程序的第一个进程，是操作系统一直连贯的
task_t* task_first_task(void) {
    return &task_manager.first_task;
}


// 设置进程就绪。
// 将进程加入就绪队列的尾部
void task_set_ready(task_t* task) {
    if (task == &task_manager.idle_task) {
        return;
    }

    list_insert_back(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

// 设置进程阻塞。
// 将进程移出就绪队列
void task_set_blocked(task_t* task) {
    if (task == &task_manager.idle_task) {
        return;
    }

    list_remove(&task_manager.ready_list, &task->run_node);
}

// 获得队列头部的进程
task_t* task_next_run(void) {
    // 队列中没有进程时，运行空闲进程
    if (list_size(&task_manager.ready_list) == 0) {
        return &task_manager.idle_task;
    }
    list_node_t* task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

// 获取当前正在运行的进程
task_t* task_current(void) {
    return task_manager.curr_task;
}

// 当前进程主动放弃CPU
int sys_sched_yield(void) {
    irq_state_t state = irq_enter_protection();

    // 读取就绪队列中头部的进程
    if (list_size(&task_manager.ready_list) > 1) {
        task_t* curr_task = task_current();

        task_set_blocked(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }

    irq_leave_protection(state);

    return 0;
}

// 分配下一个要运行的进程
void task_dispatch(void) {
    irq_state_t state = irq_enter_protection();

    task_t* to = task_next_run();

    if (to != task_manager.curr_task) {
        task_t* from = task_current();
        task_manager.curr_task = to;
        to->state = TASK_RUNNING;

        task_switch_from_to(from, to);
    }

    irq_leave_protection(state);    
}


// 当定时器中断触发时，进行一些有关进程的操作
// a. 当前进程时间片 -1
// b. 延时队列中各个进程sleeping时间 -1
void task_time_tick(void) {
    // 当前进程时间片-1，当前进程是否时间片用完？
    task_t* curr_task = task_current();

    if (--curr_task->slice_ticks == 0) {
        curr_task->slice_ticks = curr_task->time_ticks;

        task_set_blocked(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }

    // 遍历延时队列
    list_node_t* curr = list_first(&task_manager.sleep_list);
    while (curr) {
        list_node_t* next = list_node_next(curr);
        task_t* task = list_node_parent(curr, task_t, run_node);
        if (--task->sleep_ticks == 0) { // 这个进程睡够了吗？
            task_set_wakeup(task);
            task_set_ready(task);
        }
        
        curr = next;
    }

    task_dispatch();
}



void task_set_sleep(task_t* task, uint32_t ticks) {
    if (ticks == 0) {
        return;
    }

    task->sleep_ticks = ticks;
    task->state == TASK_SLEEP;
    list_insert_back(&task_manager.sleep_list, &task->run_node);
}


void task_set_wakeup(task_t* task) {
    list_remove(&task_manager.sleep_list, &task->run_node);
}


void sys_sleep(uint32_t ms) {
    irq_state_t state = irq_enter_protection();

    task_set_blocked(task_manager.curr_task);  
    task_set_sleep(task_manager.curr_task, (ms + (OS_TICKS_MS - 1)) / OS_TICKS_MS); // 向上取整

    // 现在这个进程已睡，进程开始切换！
    task_dispatch();

    irq_leave_protection(state);
}


int sys_getpid(void) {
    task_t* task = task_current();
    return task->pid;
}


int sys_fork(void) {

    return -1;
}