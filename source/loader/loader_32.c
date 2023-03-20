#include "loader.h"
#include "comm/elf.h"

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


// 解析elf文件
static uint32_t reload_elf_file(uint8_t* file_buffer) {
    Elf32_Ehdr* elf_header = (Elf32_Ehdr*)file_buffer;
    // 检查ELF文件头
    if (elf_header->e_ident[0] != 0x7f || elf_header->e_ident[1] != 'E' 
        || elf_header->e_ident[2] != 'L' || elf_header->e_ident[3] != 'F') {
            return 0;
    }

    for (int i = 0; i < elf_header->e_phnum; i++) {
        Elf32_Phdr* p_header = (Elf32_Phdr*)(file_buffer + elf_header->e_phoff) + i;

        if (p_header->p_type != PT_LOAD) {
            continue;
        }

        uint8_t* src = file_buffer + p_header->p_offset;
        uint8_t* dest = (uint8_t*)p_header->p_paddr;
        for (int j = 0; j < p_header->p_filesz; j++) {
            *dest++ = *src++;
        }

        dest = (uint8_t*)p_header->p_paddr + p_header->p_filesz;

        for (int j = 0; j < p_header->p_memsz - p_header->p_filesz; j++) {
            *dest++ = 0;
        }

        // 返回入口地址
        return elf_header->e_entry;
    }
}


// loader_32 死机
static void die(int code) {
    while (1) {  }
}

#define PDE_P           (1 << 0)    // 表项存在
#define PDE_W           (1 << 1)    // 可读
#define PDE_PS          (1 << 7)    // Page size; must be 1
#define CR4_PSE         (1 << 4)    
#define CR0_PG          (1 << 31)

void enable_page_mode(void) {
    static uint32_t page_dir[1024] __attribute__((aligned(4096))) = {
        [0] = PDE_P | PDE_W | PDE_PS | 0,

    };

    uint32_t cr4 = read_cr4();
    write_cr4(cr4 | CR4_PSE);       // 打开大页设置
    write_cr3((uint32_t)page_dir);  // 设置页目录表地址，加载于CR3中
    write_cr0(read_cr0() | CR0_PG); // 控制分页机制的比特位在CR0中
}


void load_kernel(void) {
    read_disk(100, 500, (uint8_t*)SYS_KERNEL_LOAD_ADDR); // 函数功能：从第100个扇区开始，
                                                         // 往后读500个扇区的内容（也就是250KiB）
                                                         // 因为内核放在第100个扇区（前面是loader）
                                                         // 也可以说明内核代码不会超过250KiB

    uint32_t kernel_entry_address = reload_elf_file((uint8_t*)SYS_KERNEL_LOAD_ADDR);

    if (kernel_entry_address == 0) {
        die(-1);
    }

    enable_page_mode();

    // ((void (*)(void))SYS_KERNEL_LOAD_ADDR)();
    ((void (*)(boot_info_t*))0x10000)(&boot_info);



    while (1) {  }
}
