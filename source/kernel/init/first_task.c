#include "tools/log.h"
#include "core/task.h"
#include "applib/lib_syscall.h"

int first_task_main(void) {
    int pid = getpid();
    while (1) {
        // log_printf("first task.");
        msleep(1000);
    }
    return 0;
}