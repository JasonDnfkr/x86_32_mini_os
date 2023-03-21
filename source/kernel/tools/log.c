#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "tools/klib.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"

#include <stdarg.h>

static mutex_t mutex;

#define COM1_PORT           0x3f8


void log_init(void) {
    mutex_init(&mutex, "log");
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xc7);
    outb(COM1_PORT + 4, 0x0f);
}


void log_printf(const char* fmt, ...) {
    char str_buf[128];
    kmemset(str_buf, '\0', sizeof(str_buf));

    va_list args;
    va_start(args, fmt);
    kvsprintf(str_buf, fmt, args);
    va_end(args);

    mutex_acquire(&mutex);

    const char* p = str_buf;
    while (*p != '\0') {
        // 串行接口忙，不要发送
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0) {  }
        outb(COM1_PORT, *p++);
    }

    outb(COM1_PORT, '\r');
    outb(COM1_PORT, '\n');

    mutex_release(&mutex);
}