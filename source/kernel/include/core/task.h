#ifndef TASK_H
#define TASK_H

#include "comm/types.h"
#include "cpu/cpu.h"
#include "tools/list.h"

#define TASK_NAME_SIZE          32
#define TASK_TIME_SLICE_DEFAULT 10
#define IDLE_TASK_SIZE          1024

#define TASK_FLAGS_USER         (0)
#define TASK_FLAGS_SYSTEM       (1 << 0)

// 进程控制块
typedef struct _task_t {
    // uint32_t* stack;
    enum {
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_READY,
        TASK_WAITING,
    } state;

    char name[TASK_NAME_SIZE];

    int pid;    

    list_node_t run_node;   // 对应 ready_list
    list_node_t all_node;   // 对应 task_list
    list_node_t wait_node;  // 对应 wait_list

    int sleep_ticks;        // 进程发生延时的最大时长
    int time_ticks;         // 一个进程可以维持的时间
    int slice_ticks;        // 递减到0就yield

    tss_t tss;
    int tss_sel;    // 选择子地址
} task_t;


// 进程控制块，入口地址，栈顶指针
int task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp);

// 任务切换
void task_switch_from_to(task_t* from, task_t* to);

// 进程管理器
typedef struct _task_manager_t {
    task_t* curr_task;  // 当前正在运行的进程

    list_t ready_list;  // 就绪队列
    list_t task_list;   // 已创建的进程
    list_t sleep_list;  // 延时队列

    task_t first_task;  // 操作系统一直连贯的进程
    task_t idle_task;   // 空闲进程，保证所有进程都 sleeping 的时候能够被唤起

    int app_code_sel;       // 用户程序代码段选择子
    int app_data_sel;       // 用户程序数据段选择子
} task_manager_t;

// 初始化进程队列
void task_manager_init(void);

// 进程yield，用于timer.c的定时中断中
void task_time_tick(void);

void task_first_init(void);

task_t* task_first_task(void);

void task_set_ready(task_t* task);


void task_set_blocked(task_t* task);

int sys_sched_yield(void);

// 分配下一个要运行的进程
void task_dispatch(void);

task_t* task_current(void);


void task_set_sleep(task_t* task, uint32_t ticks);

void task_set_wakeup(task_t* task);

void sys_sleep(uint32_t ms);

int sys_getpid(void);

int sys_fork(void);

#endif