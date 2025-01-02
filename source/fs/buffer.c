/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-16
 * ==================================
 */
#include "../include/buffer.h"
#include "../include/ipc.h"
#include "../include/memory.h"

static buffer_head_t bh_table[BUFFER_HASH_SIZE];
list_t dirty_blocks;

extern uint timestamp;
static mutex_t mutex;

/* 初始化链表头 */
void bb_init()
{
    for (int i=0; i<BUFFER_HASH_SIZE; i++) {
        bh_table[i].size = 0;
        list_init(&bh_table[i].head);
    }

    list_init(&dirty_blocks);

    mutex_init(&mutex);
}

static int hash_bh(int bno)
{
    return bno % BUFFER_HASH_SIZE;
}

/* 获取缓存块 */
block_buffer_t *bget_block(super_block_t* block, int bno)
{
    int hash = hash_bh(bno);

    buffer_head_t *head = &bh_table[hash];
    block_buffer_t *bb = NULL;

    /* 一个桶中最多放置100个块 */
    if (head->size >= 100) {
        list_t *bl = NULL;

        mutex_lock(&mutex);
        block_buffer_t *exp_bb = list_data(head->head.next, block_buffer_t, node);
        list_each(bl, &head->head) {
            bb = list_data(bl, block_buffer_t, node);

            if (bb->block_no == bno && bb->b_state == 1) {
                bb->exp_time = timestamp;
                return bb;
            }

            if (bb->exp_time < exp_bb->exp_time) {
                exp_bb = bb;
            }
        }

        exp_bb->block_no = bno;
        exp_bb->exp_time = timestamp;
        bb->b_state = 2;

        mutex_unlock(&mutex);

        return exp_bb;
    }

    /* 这边开始申请的内存不需要释放，一直缓存到系统关机，只是内容会被覆盖 */
    bb = kmalloc(sizeof(block_buffer_t));
    bb->block_no = bno;
    bb->exp_time = timestamp;
    bb->b_state = 0;

    bb->data = kmalloc(block->block_size);

    mutex_lock(&mutex);
    list_insert_head(&head->head, &bb->node);
    head->size++;
    mutex_unlock(&mutex);

    return bb;
}
