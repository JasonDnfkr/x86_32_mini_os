#ifndef MUTEX_H
#define MUTEX_H

#include "tools/list.h"
#include "core/task.h"

#define MUTEX_NAME_SIZE     64

typedef struct _mutex_t {
    char name[MUTEX_NAME_SIZE];
    task_t* owner;
    int locked_count;
    list_t wait_list;
} mutex_t;

void mutex_init(mutex_t* mutex, const char* name);

void mutex_acquire(mutex_t* mutex);

void mutex_release(mutex_t* mutex);


#endif