#include "tools/log.h"
#include "core/task.h"

int first_task_main(void) {
    while (1) {
        log_printf("first task.");
        sys_sleep(10000);
    }
    return 0;
}