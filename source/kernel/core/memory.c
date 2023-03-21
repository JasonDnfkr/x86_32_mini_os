#include "core/memory.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
#include "comm/cpu_instr.h"


static addr_alloc_t paddr_alloc;        // 物理地址分配器

static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(MEM_PAGE_SIZE)));

// 初始化内存分配器结构
// 作用是给定起始地址，借助位图，分配一个内存块
// 本质上是用内存块记录该起始地址的内存块内的可用信息
// 给分配器准备一个位图
// start: 起始地址
// size: 需要分配的内存大小
// page_size: 分配的页大小 (4096等)
static void addr_alloc_init(addr_alloc_t* alloc, uint8_t* bits, uint32_t start, uint32_t size, uint32_t page_size) {
    mutex_init(&alloc->mutex, "addr_alloc_t");
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

// 建立PDE到PTE的映射关系，如果 alloc 为1，则创建这个PTE
// 并返回这个PTE表项
// pde_t* page_dir: 需要建立的页表，PDE
// uint32_t vaddr： 需要建立映射的虚拟地址
// int alloc:       如果为1，则创建这个PTE
pte_t* find_pte(pde_t* page_dir, uint32_t vaddr, int alloc) {
    // 这个是二级页表的地址
    pte_t * page_table;

    pde_t *pde = &page_dir[pde_index(vaddr)];

    // 如果这个一级页表对应的二级页表，
    // 它已经存在了
    if (pde->present) {
        // 拿到二级页表的地址
        page_table = (pte_t*)pde_paddr(pde);
    }
    else {  // 这个一级页表对应的二级页表是不存在的，
            // 也就是说还没有建立映射关系
        if (alloc == 0) {
            return (pte_t*)0;
        }

        // alloc = 1, 分配一个物理页表
        uint32_t pg_paddr = addr_alloc_page(&paddr_alloc, 1);
        if (pg_paddr == 0) {
            return (pte_t*)0;
        }

        // 在一级页表中，设置为用户可读写，
        // 将被pte中设置所覆盖
        pde->v = pg_paddr | PDE_P | PTE_W | PTE_U;

        // 清空页表，防止出现异常
        // 这里虚拟地址和物理地址一一映射，所以直接写入
        page_table = (pte_t*)(pg_paddr);
        kmemset(page_table, 0, MEM_PAGE_SIZE);
    }

    return &page_table[pte_index(vaddr)];
}

// 建立内存映射
int memory_create_map(pde_t* page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm) {
    for (int i = 0; i < count; i++) {
        // log_printf("create map: v-%x p-%x, perm: %x", vaddr, paddr, perm);

        // 在页表中找 vaddr 对应的 PTE (二级页表表项)
        pte_t* pte = find_pte(page_dir, vaddr, 1);
        
        // 没有找到
        if (pte == (pte_t*)0) {
            // log_printf("create pte failed. pte == 0");
            return -1;
        }
        // 创建映射的时候，这条pte应当是不存在的。
        // 如果存在，说明可能有问题
        // log_printf("\tpte addr: %x", (uint32_t)pte);
        ASSERT(pte->present == 0);

        // 物理页(三级)的地址，和属性，加入进去
        pte->v = paddr | perm | PTE_P;
        

        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }
}


void create_kernel_table(void) {
    extern uint8_t s_text[], e_text[], s_data[], e_data[];
    extern uint8_t kernel_base[];
    
    // 地址映射表, 用于建立内核级的地址映射
    // 地址不变，但是添加了属性
    static memory_map_t kernel_map[] = {
        { kernel_base,   s_text,                         kernel_base,    PTE_W },     // 内核栈区
        { s_text,        e_text,                         s_text,         0 },         // 内核代码区
        { s_data,        (void *)(MEM_EBDA_START - 1),   s_data,         PTE_W },     // 内核数据区
        // 扩展存储空间一一映射，方便直接操作
        { (void *)MEM_EXT_START, (void *)MEM_EXT_END,    (void *)MEM_EXT_START, PTE_W },
    };

    kmemset(kernel_page_dir, 0, sizeof(kernel_page_dir));

    // 清空后，然后依次根据映射关系创建映射表
    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++) {
        memory_map_t * map = kernel_map + i;

        // 可能有多个页，建立多个页的配置
        // 简化起见，不考虑4M的情况
        int vstart = down2((uint32_t)map->vstart, MEM_PAGE_SIZE);
        int vend = up2((uint32_t)map->vend, MEM_PAGE_SIZE);
        int paddr = down2((uint32_t)map->pstart, MEM_PAGE_SIZE);
        int page_count = (vend - vstart) / MEM_PAGE_SIZE;   // 计算有多少个页

        // 建立映射关系
        memory_create_map(kernel_page_dir, vstart, paddr, page_count, map->perm);
    }
}

// 创建用户页表
uint32_t memory_create_uvm(void) {
    pde_t* page_dir = (pde_t*)addr_alloc_page(&paddr_alloc, 1);
    if (page_dir == 0) {
        return 0;
    }

    kmemset(page_dir, 0, MEM_PAGE_SIZE);

    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    for (int i = 0; i < user_pde_start; i++) {
        page_dir[i].v = kernel_page_dir[i].v;
    }

    return (uint32_t)page_dir;
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
    uint8_t * mem_free = (uint8_t *)&mem_free_start;

    // 计算1MB以上空间的空闲内存容量，并对齐的页边界
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);   // 对齐到4KB页
    log_printf("Free memory: %x, size: %x", MEM_EXT_START, mem_up1MB_free);

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    // 该部分的内存紧跟在mem_free_start开始放置
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);
    mem_free += bitmap_byte_count(paddr_alloc.size / MEM_PAGE_SIZE);

    // 到这里，mem_free应该比EBDA地址要小
    ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

    create_kernel_table();
    mmu_set_page_dir((uint32_t)kernel_page_dir);
}


int memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, uint32_t perm) {
    // 当前分配到哪个地址了？
    uint32_t curr_vaddr = vaddr;

    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;

    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = addr_alloc_page(&paddr_alloc, 1);
        if (paddr == 0) {
            log_printf("mem alloc failed. out of memory");
            return 0;
        }

        int err = memory_create_map((pde_t* )page_dir, curr_vaddr, paddr, 1, perm);
        if (err < 0) {
            log_printf("mem alloc failed in mem_create_map(), out of memory");

            // 需要取消之前建立的映射, 2023-03-21
            addr_free_page(&paddr_alloc, vaddr, i);

            return 0;
        }

        curr_vaddr += MEM_PAGE_SIZE;
    }

    return 0;
}


int memory_alloc_page_for(uint32_t addr, uint32_t size, uint32_t perm) {
    return memory_alloc_for_page_dir(task_current()->tss.cr3, addr, size, perm);
}