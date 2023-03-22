#include "cpu/cpu.h"
#include "cpu/os_cfg.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "core/syscall.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;

// selector: 段选择子
// base:     基地址
// limit:    这段内存的大小
// attr:     属性值
// 根据所给的 selector (也就是下标所指向的地址)，
// 激活这个GDT字段。
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr) {
    segment_desc_t* desc = gdt_table + selector / sizeof(segment_desc_t);

    // 判断传入的 limit 是否大于 16 位
    // 若大于，则需要将单位转换为 KiB
    // limit 单位是 KiB 还是 B
    if (limit > 0xfffff) {
        attr |= SEG_G;     // 开启G位
        limit /= 0x1000;
    }
    
    desc->limit15_0 = limit & 0xffff;
    desc->base15_0  = base & 0xffff;
    desc->base23_16 = (base >> 16) & 0xff;
    desc->attr      = attr | (((limit >> 16) & 0xf) << 8);
    desc->base31_24 = (base >> 24) & 0xff;
}


// 设置 Interrupt /Call Gate 表项
void gate_desc_set(gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr) {
    desc->offset15_0 = offset & 0xffff;
    desc->selector = selector;
    desc->attr = attr;
    desc->offset31_16 = (offset >> 16) & 0xffff;
}


void init_gdt(void) {
    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0);
    }
    
    // 数据段
    segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xffffffff, SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D);

    // 代码段
    segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xffffffff, SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D);

    // syscall 初始化。syscall gate DPL 要为3，这个描述符应该能被用户代码访问
    gate_desc_set((gate_desc_t*)(gdt_table + (SELECTOR_SYSCALL >> 3)), 
                  KERNEL_SELECTOR_CS, (uint32_t)exception_handler_syscall, GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_PARAM_COUNT);

    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}


void cpu_init(void) {
    mutex_init(&mutex, "cpu: gdt");
    init_gdt();
}


void swtch_to_tss(int tss_sel) {
    far_jump(tss_sel, 0);   // 起始地址，没有偏移
}

// 返回gdt表中一个空闲的下标所指的地址
int gdt_alloc_desc(void) {
    mutex_acquire(&mutex);

    for (int i = 1; i < GDT_TABLE_SIZE; i++) {
        segment_desc_t* desc = gdt_table + i;
        if (desc->attr == 0) {

            mutex_release(&mutex);
            
            return i * sizeof(segment_desc_t);
        }
    }

    mutex_release(&mutex);

    return -1;
}

void gdt_free_sel(int sel) {
    mutex_acquire(&mutex);

    gdt_table[sel / sizeof(segment_desc_t)].attr = 0;

    mutex_release(&mutex);
}