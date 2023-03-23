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

#define MEM_TASK_STACK_TOP      0xE0000000
#define MEM_TASK_STACK_SIZE     (MEM_PAGE_SIZE * 500)
#define MEM_TASK_ARG_SIZE       (MEM_PAGE_SIZE * 16)

#define PDE_CNT                 1024
#define PTE_CNT                 1024

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


// 拷贝用户页表
uint32_t memory_copy_uvm(uint32_t page_dir);


// 拷贝用户页表
uint32_t memory_copy_uvm2(uint32_t from, uint32_t to);


// 销毁用户页表
void memory_destroy_uvm(uint32_t page_dir);


// 给指定的虚拟内存，在指定的一级页表中，
// 构造可用空间
// uin32_t page_dir: 页表
// uin32_t vaddr:    虚拟内存
// uin32_t size:     内存大小数值，不是页数
// uin32_t perm:     权限
int memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, uint32_t perm);

// 从选择的页表中读取虚拟地址对应的物理地址
uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr);

// 在不同的进程空间中拷贝字符串
// page_dir为目标页表，当前仍为老页表
// 和xv6的函数很像，物理页不连续要逐页拷贝
int memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size);
#endif