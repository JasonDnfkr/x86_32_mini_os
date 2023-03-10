#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE      256

#define KERNEL_SELECTOR_CS  (1 * 8)
#define KERNEL_SELECTOR_DS  (2 * 8)

#define KERNEL_STACK_SIZE   (8 * 1024)    // 栈大小

// 时钟中断周期
#define OS_TICKS_MS         10

#endif