#include "fs/fs.h"
#include "comm/types.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"


// 写寄存器读取磁盘
// 使用 LBA48 模式
static void read_disk(uint32_t sector, int sector_count, uint8_t* buf) {
    outb(0x1f6, 0xE0);  // 驱动器号  选择硬盘：主盘或从盘
    outb(0x1f2, (uint8_t)(sector_count >> 8));
    outb(0x1f3, (uint8_t)(sector >> 24));
    outb(0x1f4, 0);
    outb(0x1f5, 0);

    outb(0x1f2, (uint8_t)sector_count);
    outb(0x1f3, (uint8_t)sector);
    outb(0x1f4, (uint8_t)(sector >> 8));
    outb(0x1f5, (uint8_t)(sector >> 16));

    outb(0x1f7, 0x24);

    uint16_t* data_buf = (uint16_t*)buf;
    while (sector_count--) {
        // 每次扇区读之前都要检查，等待数据就绪
        while ((inb(0x1f7) & 0x88) != 0x08) {  }

        // SECTOR_SIZE = 512
        // 每次读16位，2字节，所以是 SECTOR_SIZE / 2
        for (int i = 0; i < SECTOR_SIZE / 2; i++) {
            *data_buf++ = inw(0x1f0);
        }
    }
}

static uint8_t TEMP_ADDR[100 * 1024];
static uint8_t* temp_pos;
#define TEMP_FD         100

int sys_open(const char* name, int flags, ...) {
    if (name[0] == '/') {
        // 80: 40KiB
        read_disk(5000, 80, (uint8_t*)TEMP_ADDR);
        temp_pos = (uint8_t*)TEMP_ADDR;

        return TEMP_FD;
    }

    return -1;
}


int sys_read(int fd, char* ptr, int len) {
    if (fd == TEMP_FD) {
        kmemcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }

    return -1;
}


int sys_write(int fd, char* ptr, int len) {

    return -1;
}

// 调整文件指针
int sys_lseek(int fd, int offset, int dir) {
    if (fd == TEMP_FD) {
        temp_pos = (uint8_t*) TEMP_ADDR + offset;
        return 0;
    }

    return -1;
}


int sys_close(int fd) {
    return 0;
}

