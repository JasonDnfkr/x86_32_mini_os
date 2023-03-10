#ifndef CPU_INSTR_H
#define CPU_INSTR_H

// inb al, dx
static inline uint8_t inb(uint16_t port) {
    uint8_t rv;
    __asm__ __volatile__("inb %[p], %[v]":[v]"=a"(rv) : [p]"d"(port));
    return rv;
}


// in ax, dx
static inline uint16_t inw(uint16_t port) {
    uint16_t rv;
    __asm__ __volatile__("in %[p], %[v]":[v]"=a"(rv) : [p]"d"(port));
    return rv;
}


// outb al, dx
static inline void outb(uint16_t port, uint8_t data) {
    __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
}


// cli
static inline void cli(void) {
    __asm__ __volatile("cli");
}


// sti
static inline void sti(void) {
    __asm__ __volatile("sti");
}

// lgdt 
// 将 GDT 表加载进 内存中，即写GDTR寄存器，保存GDT表的地址
static inline void lgdt(uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } gdt;

    gdt.start31_16 = start >> 16;
    gdt.start15_0  = start & 0xffff;
    gdt.limit = size - 1; 

    __asm__ __volatile__("lgdt %[g]"::[g]"m"(gdt));
}


// lidt 
// 将 IDT 表加载进 内存中，即写IDTR寄存器，保存IDT表的地址
static inline void lidt(uint32_t start, uint32_t size) {
    struct {
        uint16_t limit;
        uint16_t start15_0;
        uint16_t start31_16;
    } idt;

    idt.start31_16 = start >> 16;
    idt.start15_0  = start & 0xffff;
    idt.limit = size - 1; 

    __asm__ __volatile__("lidt %[g]"::[g]"m"(idt));
}


static inline uint32_t read_cr0(void) {
    uint32_t cr0;

    __asm__ __volatile__("mov %%cr0, %[v]":[v]"=r"(cr0));
    
    return cr0;
}


static inline void write_cr0(uint32_t v) {
    __asm__ __volatile__("mov %[v], %%cr0"::[v]"r"(v));
}


static inline void far_jump(uint32_t selector, uint32_t offset) {
    uint32_t addr[] = { offset, selector };
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}


// hlt halt
static inline void hlt(void) {
    __asm__ __volatile__("hlt");
}

#endif