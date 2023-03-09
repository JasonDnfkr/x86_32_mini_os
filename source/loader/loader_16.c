/*
    这里的代码要在16位执行，编译后生成的代码
    都是16位的
 */

__asm__(".code16gcc");

#include "loader.h"

boot_info_t boot_info;


static void show_msg(const char* msg) {
    char c;
    while ((c = *msg++) != '\0') {
        __asm__ __volatile__(
            "mov $0xe, %%ah\n\t"
            "mov %[ch], %%al\n\t"
            "int $0x10"::[ch]"r"(c)
        );
    }
}

static void detect_memory(void) {
    uint32_t contID = 0;
    uint32_t signature;
    uint32_t bytes;

    SMAP_entry_t smap_entry;

    show_msg("trying to detect memory: ");

    boot_info.ram_region_count = 0;
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++) {
        SMAP_entry_t* entry = &smap_entry;

        __asm__ __volatile__ ("int  $0x15" 
				: "=a"(signature), "=c"(bytes), "=b"(contID)
				: "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry));
    
        if (signature != 0x534D4150) {
            show_msg("failed\r\n");
            return;
        }

        if (bytes > 20 && (entry->ACPI & 0x0001) == 0) {
                continue;
        }

        if (entry->Type == 1) {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++; 
        }

        if (contID == 0) {
            break;
        }
    }

    show_msg("ok.\r\n");
}


uint16_t gdt_table[][4] = {
    { 0, 0, 0, 0 },                     // GDT 第一个表项：全为0
    { 0xffff, 0x0000, 0x9a00, 0x00cf }, // GDT 第二个表项：代码段
    { 0xffff, 0x0000, 0x9200, 0x00cf }, // GDT 第三个表项：数据段
};


static void enter_protected_mode(void) {
    // 关中断
    cli();

    // 开启 A20 地址线
    uint8_t v = inb(0x92);
    outb(0x92, v | 0x02);

    // 加载 gdt 表
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));

    // 写 cr0 寄存器
    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | (1 << 0));

    // 远跳转至保护模式入口
    far_jump(8, (uint32_t)protected_mode_entry);
}

void loader_entry(void) {
    show_msg("....loading....\n\r");
    detect_memory();
    enter_protected_mode();

    while (1) { }
}

 