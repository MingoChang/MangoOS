/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-16
 * ==================================
 */
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "type.h"
#include "list.h"
#include "ext2.h"

#define BUFFER_HASH_SIZE 10

typedef struct _buffer_head_t
{
    int size;       /* 该连表中目前有多少个缓冲块 */
    list_t head;    /* 缓冲块头 */
}buffer_head_t;

typedef struct _block_buffer_t
{
    uint block_no;      /* 块号 */
    uchar *data;        /* 数据缓冲区 */
    uchar b_state;      /* 状态 */
    uint exp_time;      /* 过期时间 */
    list_t node;        /* 链表节点 */
    list_t dirty_node;  /* 链表节点 */
}block_buffer_t;

void bb_init();
block_buffer_t *bget_block(super_block_t* block, int bno);

#endif
