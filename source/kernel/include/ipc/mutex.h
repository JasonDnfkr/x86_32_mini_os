#ifndef MUTEX_H
#define MUTEX_H

#include "tools/list.h"
#include "core/task.h"

typedef struct _mutex_t {
    task_t* owner;

    int locked_count;   // 

    list_t wait_list;
} mutex_t;

void mutex_init(mutex_t* mutex);

void mutex_acquire(mutex_t* mutex);

void mutex_release(mutex_t* mutex);


#endif