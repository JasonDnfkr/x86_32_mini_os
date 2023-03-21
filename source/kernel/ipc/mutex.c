#include "ipc/mutex.h"
#include "cpu/irq.h"
#include "tools/klib.h"

void mutex_init(mutex_t* mutex, const char* name) {
    kstrcpy(mutex->name, name);
    mutex->locked_count = 0;
    mutex->owner = (task_t*) 0;
    list_init(&mutex->wait_list);
}

void mutex_acquire(mutex_t* mutex) {
    irq_state_t state = irq_enter_protection();

    task_t* curr = task_current();
    if (mutex->locked_count == 0) {
        mutex->locked_count = 1;
        mutex->owner = curr;
    }
    else if (mutex->owner == curr) {
        mutex->locked_count++;
    }
    else {
        task_set_blocked(curr);
        list_insert_back(&mutex->wait_list, &curr->wait_node);  // 把当前进程（自己）放入mutex的等待队列
        task_dispatch();
    }

    irq_leave_protection(state);
}

void mutex_release(mutex_t* mutex) {
    irq_state_t state = irq_enter_protection();

    task_t* curr = task_current();
    if (mutex->owner == curr) {
        if (--mutex->locked_count == 0) {
            mutex->owner = (task_t*)0;

            if (list_size(&mutex->wait_list)) {
                list_node_t* node = list_remove_front(&mutex->wait_list);
                task_t* task = list_node_parent(node, task_t, wait_node);
                task_set_ready(task);

                mutex->owner = task;
                mutex->locked_count = 1;

                task_dispatch();
            }
        }
    }

    irq_leave_protection(state);
}