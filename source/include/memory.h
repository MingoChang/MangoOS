/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-28
 * ==================================
 */
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "type.h"

#define USED    0xAA99BB88
#define FREE    0x88BB99AA

#pragma pack(1)

typedef struct _mem_block_t
{
    uint used;
    uint size;
    uchar is_head;
    struct _mem_block_t *prev;
    struct _mem_block_t *next;
}mem_block_t;

#pragma pack()

void mem_init();
void* kmalloc(uint size);
void kfree(void* address);

#endif
