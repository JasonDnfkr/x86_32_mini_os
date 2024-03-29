#ifndef MMU_H
#define MMU_H

#include "comm/types.h"
#include "comm/cpu_instr.h"

#define PTE_P   (1 << 0)    // present
#define PDE_P   (1 << 0)    // present
#define PDE_W   (1 << 1)    // read / write
#define PTE_W   (1 << 1)    // read / write
#define PTE_U   (1 << 2)    // user
#define PDE_U   (1 << 2)    // user

#pragma pack(1)

// Page Directory Entry
typedef union _pde_t {
    uint32_t v;
    struct {
        uint32_t present :       1;             // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed :      1;             // 5 (A) Accessed
        uint32_t :               1;             // 6 Ignored;
        uint32_t ps :            1;             // 7 (PS)
        uint32_t :               4;             // 11:8 Ignored
        uint32_t phy_pt_addr :  20;             // 
    };
} pde_t;


// Page-Table Entry
typedef union _pte_t {
    uint32_t v;
    struct {
        uint32_t present :       1;             // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed :      1;             // 5 (A) Accessed;
        uint32_t dirty :         1;             // 6 (D) Dirty
        uint32_t pat :           1;             // 7 PAT
        uint32_t global :        1;             // 8 (G) Global
        uint32_t :               3;             // Ignored
        uint32_t phy_page_addr : 20;            // 高20位物理地址
    };
} pte_t;

#pragma pack()

// 返回vaddr在页目录中的索引
// 只取高10位
static inline uint32_t pde_index (uint32_t vaddr) {
    int index = (vaddr >> 22); // 只取高10位
    return index;
}


// 获取pde中的地址字段
static inline uint32_t pde_paddr(pde_t * pde) {
    return pde->phy_pt_addr << 12;
}


// 返回vaddr在页表中的索引
// 取中间10位
static inline int pte_index(uint32_t vaddr) {
    return (vaddr >> 12) & 0x3FF;   // 取中间10位
}

// 获取pte中的物理地址
static inline uint32_t pte_paddr(pte_t * pte) {
    return pte->phy_page_addr << 12;
}

static inline void mmu_set_page_dir(uint32_t paddr) {
    write_cr3(paddr);
}


static inline uint32_t get_pte_perm(pte_t* pte) {
    return (pte->v & 0x3ff);
}


#endif