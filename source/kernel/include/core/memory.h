#ifndef MEMORY_H
#define MEMORY_H

#include "comm/types.h"
#include "tools/bitmap.h"
#include "ipc/mutex.h"
#include "comm/boot_info.h"

#define MEM_EXT_START           (1 * 1024 * 1024)
#define MEM_PAGE_SIZE           (4096)
#define MEM_EBDA_START          (0x80000) // 显存位置，不应该被使用
#define MEMORY_TASK_BASE        (0x80000000)
#define MEM_EXT_END             (127 * 1024 * 1024)

#define PDE_CNT                 1024

// 内存分配器
// mutex: 互斥锁
// bitmap: 位图
// start: 起始地址
// size: 需要分配的内存大小
// page_size: 分配的页大小 (4096等)
typedef struct _addr_alloc_t {
    mutex_t mutex;
    bitmap_t bitmap;

    uint32_t start;
    uint32_t size;
    uint32_t page_size; // 页大小
} addr_alloc_t;


typedef struct _memory_map_t {
    void* vstart;   // 虚拟地址的起始处
    void* vend;     // 虚拟地址的结束处
    void* pstart;   // 这一块区域的物理地址起始处
    uint32_t perm;  // 特权控制字段
} memory_map_t;


void memory_init(boot_info_t* boot_info);

// 进程创建时，用于给task_t中的CR3创建页表，并给页表分配内存
uint32_t memory_create_uvm(void);

// 给当前进程建立页表映射
// 从指定的地址vaddr开始，需要建立的内存大小为size，权限为perm
// 该size不是页表数量，是内存数值大小
int memory_alloc_page_for(uint32_t vaddr, uint32_t size, uint32_t perm);

// 分配一页内存
uint32_t memory_alloc_page(void);

// 销毁一页内存
void memory_free_page(uint32_t vaddr);

#endif