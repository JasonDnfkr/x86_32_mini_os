#ifndef KLIB_H
#define KLIB_H

#include "comm/types.h"

#include <stdarg.h>

// 向上对齐到页边界
static inline uint32_t up2 (uint32_t size, uint32_t bound) {
    return (size + bound - 1) & ~(bound - 1);
}

// 向下对齐到界边界
static inline uint32_t down2 (uint32_t size, uint32_t bound) {
    return size & ~(bound - 1);
}


void kstrcpy(char* dest, const char* src);

void kstrncpy(char* dest, const char* src, int size);

int kstrncmp(const char* s1, const char* s2, int size);

int kstrlen(const char* str);

void kmemcpy(void* dest, void* src, int size);

void kmemset(void* dest, uint8_t v, int size);

int kmemcmp(void* d1, void* d2, int size);

void kitoa(char* buf, int num, int base);

void ksprintf(char* buf, const char* fmt, ...);

void kvsprintf(char* buf, const char* fmt, va_list args);


#ifndef RELEASE

#define ASSERT(expr)    \
    if (!(expr)) pannic(__FILE__, __LINE__, __func__, #expr)
void pannic(const char* file, int line, const char* func, const char* cond);

#else
#define ASSERT(expr)     ((void)0)

#endif

#endif