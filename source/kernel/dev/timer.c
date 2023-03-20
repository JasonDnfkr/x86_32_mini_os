#include "comm/types.h"
#include "dev/timer.h"
#include "comm/cpu_instr.h"
#include "cpu/os_cfg.h"
#include "cpu/irq.h"
#include "core/task.h"

// 系统运行时间
static uint32_t sys_tick;

// 定时中断到达后增加计时
void do_handler_timer(exception_frame_t* frame) {
    sys_tick++;

    // 中断达到后，需要通知 8259 芯片，当前的中断已经处理完毕
    // 需要发个命令给它，这样 8259 才能响应后面的中断
    pic_send_eoi(IRQ0_TIMER);

    task_time_tick();
}


static void init_pit(void) {
    uint32_t reload_count = PIT_OSC_FREQ * OS_TICKS_MS / 1000;

    // 端口，对定时器0进行配置
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL | PIT_LOAD_LOHI | PIT_MODE3);

    // 设置的时间值是 reload_count，
    // 定时器周期，reload_count 减到 0 后，把这个值重设
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xff);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xff);

    irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_timer);
    irq_enable(IRQ0_TIMER);
}

void timer_init(void) {
    sys_tick = 0;
    init_pit();
}