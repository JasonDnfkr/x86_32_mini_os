#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
#include "comm/cpu_instr.h"

#define PDE_CNT         1024

static addr_alloc_t paddr_alloc;        // 物理地址分配结构

// 初始化内存分配器结构
// 作用是给定起始地址，借助位图，分配一个内存块
// 本质上是用内存块记录该起始地址的内存块内的可用信息
// 给分配器准备一个位图
// start: 起始地址
// size: 需要分配的内存大小
// page_size: 分配的页大小 (4096等)
static void addr_alloc_init(addr_alloc_t* alloc, uint8_t* bits, uint32_t start, uint32_t size, uint32_t page_size) {
    mutex_init(&alloc->mutex);
    alloc->start = start;
    alloc->size = size;
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}

// 申请内存页
// 根据内存分配器中的信息，（起始页，该内存块可用大小，该块中分配的页大小）
// 来申请内存
// int page_count: 页数
static uint32_t addr_alloc_page(addr_alloc_t* alloc, int page_count) {
    uint32_t addr = 0;

    mutex_acquire(&alloc->mutex);

    // 从位图视角中，找到一个连续的可以分配的储存空间，
    // 将起始下标返回给 page_index
    // 
    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count);
    if (page_index >= 0) {
        addr = alloc->start + page_index * alloc->page_size;
    }

    mutex_release(&alloc->mutex);

    return addr;
}


// 释放内存页
// 内存分配器
// uint32_t addr: 释放的内存页的起始地址
// int page_count: 需要释放的页数
static void addr_free_page(addr_alloc_t* alloc, uint32_t addr, int page_count) {
    mutex_acquire(&alloc->mutex);

    uint32_t pg_index = (addr - alloc->start) / alloc->page_size;
    bitmap_set_bit(&alloc->bitmap, pg_index, page_count, 0);

    mutex_release(&alloc->mutex);
}


static void show_mem_info (boot_info_t * boot_info) {
    log_printf("mem region:");
    for (int i = 0; i < boot_info->ram_region_count; i++) {
        log_printf("[%d]: %x - %x", i,
                    boot_info->ram_region_cfg[i].start,
                    boot_info->ram_region_cfg[i].size);
    }
    log_printf("\n");
}


// 获取可用的物理内存大小
static uint32_t total_mem_size(boot_info_t * boot_info) {
    int mem_size = 0;

    // 简单起见，暂不考虑中间有空洞的情况
    for (int i = 0; i < boot_info->ram_region_count; i++) {
        mem_size += boot_info->ram_region_cfg[i].size;
    }
    return mem_size;
}


static void test_alloc(void) {
    addr_alloc_t addr_alloc;

    uint8_t bits[8];
    addr_alloc_init(&addr_alloc, bits, 0x1000, 64 * 4096, 4096);

    for (int i = 0; i < 32; i++) {
        uint32_t addr = addr_alloc_page(&addr_alloc, 2);
        log_printf("alloc addr: %x\n", addr);
    }
    uint32_t addr = 0x1000;
    for (int i = 0; i < 32; i++) {
        addr += 4096 * 2;
        addr_free_page(&addr_alloc, addr, 2);
    }
}


// 对整个操作系统的内存管理
// 初始化
void memory_init(boot_info_t* boot_info) {
    // test_alloc();

    // 1MB内存空间起始，在链接脚本中定义
    extern uint8_t * mem_free_start;

    log_printf("mem init.");
    show_mem_info(boot_info);

    // 在内核数据后面放物理页位图
    uint8_t * mem_free = (uint8_t *)&mem_free_start;   // 2022年-10-1 经同学反馈，发现这里有点bug，改了下

    // 计算1MB以上空间的空闲内存容量，并对齐的页边界
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);   // 对齐到4KB页
    log_printf("Free memory: %x, size: %x", MEM_EXT_START, mem_up1MB_free);

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    // 该部分的内存仅跟在mem_free_start开始放置
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);
    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE);

    // 到这里，mem_free应该比EBDA地址要小
    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    // create_kernel_table();
}
