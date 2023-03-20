// #ifndef MMU_H
// #define MMU_H

// #include "comm/types.h"
// #include "comm/cpu_instr.h"

// #define PTE_P   (1 << 0)
// #define PDE_P   (1 << 0)


// // Page Directory Entry
// typedef union _pde_t {
//     uint32_t v;
//     struct {
//         uint32_t present        : 1;       // must be 1 to reference a page table
//         uint32_t write_enable   : 1;  // read / write. if 0, writes may not be allowed to the 4-MByte region controlled by this entry
//         uint32_t user_mode_acc  : 1; // user / supervisor. if 0, user-mode accesses are not allowed to the 4-MByte region controlled by this entry
//         uint32_t write_through  : 1;
//         uint32_t cache_disable  : 1;
//         uint32_t accessed       : 1;
//         uint32_t                : 1;
//         uint32_t ps             : 1;
//         uint32_t                : 4;
//         uint32_t phy_pt_addr    : 20;

//     };
// } pde_t;


// // Page-Table Entry
// typedef union _pte_t {
//     uint32_t v;
//     struct {
//         uint32_t present        : 1;        // must be 1 to reference a page table
//         uint32_t write_enable   : 1;        // read / write. if 0, writes may not be allowed to the 4-MByte region controlled by this entry
//         uint32_t user_mode_acc  : 1;        // user / supervisor. if 0, user-mode accesses are not allowed to the 4-MByte region controlled by this entry
//         uint32_t write_through  : 1;
//         uint32_t cache_disable  : 1;
//         uint32_t accessed       : 1;
//         uint32_t dirty          : 1;
//         uint32_t pat            : 1;
//         uint32_t global         : 1;
//         uint32_t                : 3;
//         uint32_t phy_page_addr  : 20;

//     };
// } pte_t;


// // 获取虚拟地址高10位
// static inline uint32_t pde_index(uint32_t vaddr) {
//     int index = (vaddr >> 22);
//     return index;
// }

// // 获取虚拟地址中间10位
// static inline uint32_t pte_index(uint32_t vaddr) {
//     return (vaddr >> 12) & 0x3ff;
// }

// // 返回 pde 表项的起始部分(高位地址)
// static inline uint32_t pde_paddr(pde_t* pde) {
//     return pde->phy_pt_addr << 12;
// }

// // 返回 pte 表项的起始部分(高位地址)
// static inline uint32_t pte_paddr(pte_t* pte) {
//     return pte->phy_page_addr << 12;
// }

// static inline void mmu_set_page_dir(uint32_t paddr) {
//     write_cr3(paddr);
// }




// #endif