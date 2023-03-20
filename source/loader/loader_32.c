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
    Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)file_buffer;
    // 检查ELF文件头
    if ((elf_hdr->e_ident[0] != ELF_MAGIC) || (elf_hdr->e_ident[1] != 'E') 
        || (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F')) {
            return 0;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    for (int i = 0; i < elf_hdr->e_phnum; i++) {
        Elf32_Phdr* p_hdr = (Elf32_Phdr*)(file_buffer + elf_hdr->e_phoff) + i;

        if (p_hdr->p_type != PT_LOAD) {
            continue;
        }

        uint8_t* src = file_buffer + p_hdr->p_offset;
        uint8_t* dest = (uint8_t*)p_hdr->p_paddr;
        for (int j = 0; j < p_hdr->p_filesz; j++) {
            *dest++ = *src++;
        }

        dest = (uint8_t*)p_hdr->p_paddr + p_hdr->p_filesz;

        for (int j = 0; j < p_hdr->p_memsz - p_hdr->p_filesz; j++) {
            *dest++ = 0;
        }
    }

    // 返回入口地址
    return elf_hdr->e_entry;
}


// loader_32 死机
static void die(int code) {
    while (1) {  }
}


void enable_page_mode (void) {
#define PDE_P			(1 << 0)
#define PDE_PS			(1 << 7)
#define PDE_W			(1 << 1)
#define CR4_PSE		    (1 << 4)
#define CR0_PG		    (1 << 31)

    // 使用4MB页块，这样构造页表就简单很多，只需要1个表即可。
    // 以下表为临时使用，用于帮助内核正常运行，在内核运行起来之后，将重新设置
    static uint32_t page_dir[1024] __attribute__((aligned(4096))) = {
        [0] = PDE_P | PDE_PS | PDE_W,			// PDE_PS，开启4MB的页
    };

    // 设置PSE，以便启用4M的页，而不是4KB
    uint32_t cr4 = read_cr4();
    write_cr4(cr4 | CR4_PSE);

    // 设置页表地址
    write_cr3((uint32_t)page_dir);

    // 开启分页机制
    write_cr0(read_cr0() | CR0_PG);
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
    ((void (*)(boot_info_t*))kernel_entry_address)(&boot_info);



    while (1) {  }
}
