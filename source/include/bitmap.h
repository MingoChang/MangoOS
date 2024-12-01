/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-23
 * ==================================
 */
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "type.h"

#define PAGE_SIZE 4096
#define BITMAP_START 0x10400
#define FREE_MEM_START 0x601000

#pragma pack(1)

/* 内存布局结构体 */
typedef struct _mem_layout_t
{
    long long start;
    long long size;
    uint type;
}mem_layout_t;

/* 物理内存位图 */
typedef struct _bit_map_t
{
    int nbits; /* 位图占的比特数 */
    uchar *base;
}bit_map_t;

#pragma pack()

void bitmap_init();
int bitmap_alloc_bits(int count);
void bitmap_free_bits(uint addr, int count);

#endif
