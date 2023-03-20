#ifndef TIMER_H
#define TIMER_H

#define PIT_OSC_FREQ         1193182  // 定时器振荡器时钟
#define PIT_COMMAND_MODE_PORT   0x43  // 命令模式端口
#define PIT_CHANNEL0_DATA_PORT  0x40  // 数据模式端口

#define PIT_CHANNEL         (0 << 6)
#define PIT_LOAD_LOHI       (3 << 4)
#define PIT_MODE3           (3 << 1)

void timer_init(void);

void exception_handler_timer(void);

#endif