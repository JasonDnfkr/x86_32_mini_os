#include "ipc/sem.h"
#include "core/task.h"
#include "cpu/irq.h"

void sem_init(sem_t* sem, int init_count) {
    sem->count = init_count;
    list_init(&sem->wait_list);
}


void sem_wait(sem_t* sem) {
    irq_state_t state = irq_enter_protection();

    // 进程获得了一个信号，且不为0
    if (sem->count > 0) {
        sem->count--;
    }
    else {
        task_t* curr = task_current();
        task_set_blocked(curr);
        list_insert_back(&sem->wait_list, &curr->wait_node);
        
        task_dispatch();
    }

    irq_leave_protection(state);
}


void sem_notify(sem_t* sem) {
    irq_state_t state = irq_enter_protection();

    // 如果有进程在等待队列中
    if (list_size(&sem->wait_list)) {
        list_node_t* node = list_remove_front(&sem->wait_list);
        task_t* task = list_node_parent(node, task_t, wait_node);
        task_set_ready(task);

        task_dispatch();
    }
    else {  // 发了一个信号，但没有人接收，先增加
        sem->count++;
    }

    irq_leave_protection(state);
}

int sem_count(sem_t* sem) {
    irq_state_t state = irq_enter_protection();

    int count = sem->count;
    
    irq_leave_protection(state);

    return count;
}