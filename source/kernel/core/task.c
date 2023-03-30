#include "core/task.h"
#include "tools/klib.h"
#include "cpu/os_cfg.h"
#include "cpu/cpu.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "core/memory.h"
#include "cpu/mmu.h"
#include "core/syscall.h"
#include "comm/elf.h"
#include "fs/fs.h"

// 全局的进程队列
static task_manager_t task_manager;

// 进程数组
static task_t task_table[TASK_NR];

static mutex_t task_table_mutex;

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

    task->parent = (task_t*)0;

    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    irq_state_t state = irq_enter_protection();


    task->pid = (uint32_t)task;

    // task_set_ready(task);

    list_insert_back(&task_manager.task_list, &task->all_node);

    irq_leave_protection(state);

    return 0;
}


void task_uninit(task_t* task) {
    if (task->tss_sel) {
        gdt_free_sel(task->tss_sel);
    }

    // 把内核栈释放掉
    if (task->tss.esp0) {
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
    }

    if (task->tss.cr3) {
        memory_destroy_uvm(task->tss.cr3);
    }

    kmemset(task, 0, sizeof(task_t));
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

void task_start(task_t* task) {
    irq_state_t state = irq_enter_protection();

    task_set_ready(task);
    
    irq_leave_protection(state);
}


void task_manager_init(void) {
    kmemset(task_table, 0, sizeof(task_table));
    mutex_init(&task_table_mutex, "task_table");

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

    // task_manager.curr_task = &task_manager.idle_task;

    task_start(&task_manager.idle_task);
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

    task_start(&task_manager.first_task);
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



// 分配 task
static task_t* alloc_task(void) {
    task_t* task = (task_t*)0;

    mutex_acquire(&task_table_mutex);

    for (int i = 0; i < TASK_NR; i++) {
        task_t* curr = task_table + i;

        if (curr->name[0] == '\0') {
            task = curr;
            break;
        }
    }

    mutex_release(&task_table_mutex);

    return task;
}

// 释放 task
static void free_task(task_t* task) {
    mutex_acquire(&task_table_mutex);

    task->name[0] = '\0';

    mutex_release(&task_table_mutex);
}




int sys_fork(void) {
    // 获取父进程
    task_t* parent_task = task_current();

    task_t* child_task = alloc_task();
    if (child_task == (task_t*)0) {
        goto fork_failed;
    }

    // 获取进程的 寄存器结构的起始地址
    syscall_frame_t * frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));

    // 子进程会的eip会直接指向ret那一条，而不是lcalll
    // 这意味着，它的特权级是在3的
    // 这又意味着，此时它的用户栈空间，有5个syscall的参数
    // 需要把esp指针调回去
    int err = task_init(child_task, 
                        parent_task->name, 
                        TASK_FLAGS_USER, 
                        frame->eip, 
                        frame->esp + sizeof(uint32_t) * SYSCALL_PARAM_COUNT); // 这里的esp是esp3
    if (err < 0) {
        goto fork_failed;
    }

    // 从父进程的栈中取部分状态，然后写入tss。
    // 注意检查esp, eip等是否在用户空间范围内，不然会造成page_fault
    tss_t * tss = &child_task->tss;
    tss->eax = 0;                       // 子进程返回0
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task;

    // 复制父进程的内存空间到子进程
    if ((tss->cr3 = memory_copy_uvm2(parent_task->tss.cr3, tss->cr3)) < 0) {
        goto fork_failed;
    }

    task_start(child_task);

    return child_task->pid;

fork_failed:
    if (child_task) {
        task_uninit(child_task);
        free_task(child_task);
    }
    return -1;
}


static int load_phdr(int fd, Elf32_Phdr* phdr, uint32_t page_dir) {

    // 分配空间
    int err = memory_alloc_for_page_dir(page_dir, phdr->p_vaddr, 
                                        phdr->p_memsz, PTE_P | PTE_U | PTE_W);
    if (err < 0) {
        log_printf("load_phdr: no memory");
        return -1;
    }

    // 调整当前的读写位置，指针指向当前 program 的开头
    if (sys_lseek(fd, phdr->p_offset, 0) < 0) {
        log_printf("load_phdr: read file failed");
        return -1;
    }

    uint32_t vaddr = phdr->p_vaddr;
    uint32_t size = phdr->p_filesz;

    while (size > 0) {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;

        uint32_t paddr = memory_get_paddr(page_dir, vaddr);

        // 注意，这里用的页表仍然是当前的
        if (sys_read(fd, (char *)paddr, curr_size) <  curr_size) {
            log_printf("read file failed");
            return -1;
        }

        size -= curr_size;
        vaddr += curr_size;
    }

    return 0;
}


static uint32_t load_elf_file(task_t* task, const char* name, uint32_t page_dir) {
    Elf32_Ehdr elf_hdr;     // 创建elf_hdr内存空间，用于读取elf头
    Elf32_Phdr elf_phdr;    // 创建phdr内存空间，用于读取phdr头

    int fd = sys_open(name, 0);
    if (fd < 0) {
        log_printf("open failed: %s", name);

        goto load_failed;
    }

    int cnt = sys_read(fd, (char*)&elf_hdr, sizeof(elf_hdr));
    if (cnt < sizeof(Elf32_Ehdr)) {
        log_printf("elf hdr smaller than sizeof(Elf32_Ehdr). size = %d", cnt);

        goto load_failed;
    }

    // ELF MAGIC NUMBER 检查 
    if ((elf_hdr.e_ident[0] != ELF_MAGIC) || (elf_hdr.e_ident[1] != 'E')
        || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')) {
        log_printf("check elf indent failed.");

        goto load_failed;
    }

    // 必须是可执行文件和针对386处理器的类型，且有入口
    // if ((elf_hdr.e_type != ET_EXEC) || (elf_hdr.e_machine != ET_386) || (elf_hdr.e_entry == 0)) {
    //     log_printf("check elf type or entry failed.");

    //     goto load_failed;
    // }

    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0)) {
        log_printf("none programe header");
        goto load_failed;
    }

    // 加载程序头，将内容拷贝到相应的位置
    uint32_t e_phoff = elf_hdr.e_phoff;
    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) {
        // 读取偏移量
        if (sys_lseek(fd, e_phoff, 0) < 0) {
            log_printf("read file failed");

            goto load_failed;
        }

        // 读取程序头后解析，这里不用读取到新进程的页表中，因为只是临时使用下
        cnt = sys_read(fd, (char *)&elf_phdr, sizeof(Elf32_Phdr));
        if (cnt < sizeof(Elf32_Phdr)) {
            log_printf("read file failed");
            goto load_failed;
        }

        // 简单做一些检查
        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE)) {
           continue;
        }

        // 加载当前程序头
        int err = load_phdr(fd, &elf_phdr, page_dir);
        if (err < 0) {
            log_printf("load program hdr failed");
            goto load_failed;
        }

        sys_close(fd);
        return elf_hdr.e_entry;
    }

    return 0;

load_failed:
    if (fd >= 0) {
        sys_close(fd);
    }
    return -1;
}


static int copy_args(char* to, uint32_t page_dir, int argc, char** argv) {
    task_args_t task_args;
    task_args.argc = argc;
    task_args.argv = (char**)(to + sizeof(task_args_t));

    // 复制各项参数, 跳过task_args和参数表
    // 各argv参数写入的内存空间
    char* dest_arg = to + sizeof(task_args_t) + sizeof(char *) * (argc);   // 留出结束符

    // argv表
    char** dest_argv_tb = (char **)memory_get_paddr(page_dir, (uint32_t)(to + sizeof(task_args_t)));
    ASSERT(dest_argv_tb != 0);

    for (int i = 0; i < argc; i++) {
        char* from = argv[i];
        int len = kstrlen(from) + 1;    // '\0'
        int err = memory_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)from, len);
        ASSERT(err >= 0);

        // 关联ar
        dest_argv_tb[i] = dest_arg;

        // 记录下位置后，复制的位置前移
        dest_arg += len;
    }

    return memory_copy_uvm_data((uint32_t)to, page_dir, (uint32_t)&task_args, sizeof(task_args));
}


int sys_execve(char* name, char** argv, char** env) {
    task_t* task = task_current();

    // 更新进程的名字
    kstrncpy(task->name, get_file_name(name), TASK_NAME_SIZE);

    // 获取原页表，需要销毁
    uint32_t old_page_dir = task->tss.cr3;
    // 创建一个新页表
    uint32_t new_page_dir = memory_create_uvm();
    if (!new_page_dir) {
        goto execve_failed;
    }

    // 加载elf文件的入口地址      给哪个进程加载，路径，页表是哪个
    uint32_t entry = load_elf_file(task, name, new_page_dir);
    if (entry == 0) {
        goto execve_failed;
    }

    // 分配用户栈空间
    uint32_t stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE;
    
    int err = memory_alloc_for_page_dir(new_page_dir, 
                                        MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE, 
                                        MEM_TASK_STACK_SIZE, 
                                        PTE_P | PTE_U | PTE_W);

    if (err < 0) {
        goto execve_failed;
    }

    // 复制参数，写入到栈顶的后边
    int argc = strings_count(argv);
    err = copy_args((char*)stack_top, new_page_dir, argc, argv);
    if (err < 0) {
        goto execve_failed;
    }

    
    // 因为要从内核返回，这里把内核栈里的EIP 等 其他寄存器 内容修改掉
    // 这样在 iret 返回时，就可以直接去新的地址，和新的环境
    syscall_frame_t * frame = (syscall_frame_t *)(task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = 0;
    frame->ebx = 0;
    frame->ecx = 0;
    frame->edx = 0;
    frame->esi = 0;
    frame->edi = 0;
    frame->ebp = 0;
    frame->eflags = EFLAGS_DEFAULT | EFLAGS_IF;
    frame->esp = stack_top - sizeof(uint32_t) * SYSCALL_PARAM_COUNT; // 要执行retf $(5 * 4)

    task->tss.cr3 = new_page_dir;

    // 修改进程的 TR寄存器，用新页表
    mmu_set_page_dir(new_page_dir);

    memory_destroy_uvm(old_page_dir);

    // 此时原进程的地址 0x80000000已经没有东西了



    return 0;


execve_failed:
    if (new_page_dir) {
        task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(old_page_dir);

        memory_destroy_uvm(new_page_dir);
    }


    return -1;
}


int sys_exit(int status) {


    irq_state_t state = irq_enter_protection();

    task_t* curr = task_current();
    curr->state = TASK_ZOMBIE;

    task_set_blocked(curr);
    task_dispatch();

    irq_leave_protection(state);

    return 0;
}