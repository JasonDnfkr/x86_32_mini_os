#include "loader.h"

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


void load_kernel(void) {
    read_disk(100, 500, (uint8_t*)SYS_KERNEL_LOAD_ADDR); // 函数功能：从第100个扇区开始，
                                                         // 往后读500个扇区的内容（也就是250KiB）
                                                         // 因为内核放在第100个扇区（前面是loader）
                                                         // 也可以说明内核代码不会超过250KiB
    // ((void (*)(void))SYS_KERNEL_LOAD_ADDR)();
    ((void (*)(boot_info_t*))SYS_KERNEL_LOAD_ADDR)(&boot_info);



    while (1) {  }
}
