#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

typedef struct _bitmap_t {
    int bit_count;
    uint8_t* bits;
} bitmap_t;

int bitmap_byte_count(int bit_count);

void bitmap_init(bitmap_t* bitmap, uint8_t* bits, int count, int init_bit);

// 获取状态值
int bitmap_get_bit(bitmap_t* bitmap, int index);

// 对某一个位设置
// index 开头，连续 count 位设置成 bit_value
void bitmap_set_bit(bitmap_t* bitmap, int index, int count, int bit_value);

// 判断该位是否？
int bitmap_is_set(bitmap_t* bitmap, int index);

// 连续多个count个 位的值为bit的
int bitmap_alloc_nbits(bitmap_t* bitmap, int bit, int count);

#endif