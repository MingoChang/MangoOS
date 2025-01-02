/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-28
 * ==================================
 */
#include "../include/memory.h"
#include "../include/page.h"
#include "../include/string.h"
#include "../include/log.h"

static mem_block_t *free_list = NULL;

void mem_init()
{
    page_init();

    free_list = (mem_block_t*)alloc_pages(8);
    free_list->size = 8 * PAGE_SIZE;
    free_list->used = FREE;
    free_list->is_head = 1;
    free_list->prev = NULL;
    free_list->next = NULL;

}

/* 分配内核内存，内核虚拟内存==物理内存 */
void* kmalloc(uint size)
{
    /* 多申请一个内存头空间 */
    size += MEM_HSIZE;

    /* 最少申请8个字节, 最多8页内存(加内存头) */
    if (size < 8 || size > 8 * PAGE_SIZE) {
        kprintf("The size %d is not allow to alloc, the max alloc size is %d", size, 8 * PAGE_SIZE);
        return NULL;
    }

    mem_block_t *current = free_list;
    mem_block_t *prev = current;

    /* 遍历空闲链表寻找合适的内存块 */
    while (current) {
        if (current->size >= size) {
            /* 内存分配了还有多， 多的部分作为一个新的空闲块 */
            if (current->size > size + MEM_HSIZE) {
                mem_block_t *new_block = (mem_block_t*)((uint)current + size);            
                new_block->size = current->size - size;
                new_block->prev = current;
                new_block->next = current->next;
                if (current->next) {
                    current->next->prev = new_block;
                }
                new_block->used = FREE;
                new_block->is_head = 0;

                current->size = size;
                current->next = new_block;
            }

            current->used = USED;

            /* 当前节点已经分配，从空闲链表中删除 */
            if (current->prev) {
                current->prev->next = current->next;
                if (current->next) {
                    current->next->prev = current->prev;
                }
            } else {
                free_list = current->next;
                if (current->next) {
                    current->next->prev = NULL;
                }
            }

            return (void*)((uchar*)current + MEM_HSIZE);
        }

        prev = current;
        current = current->next;
    }

    /* 现有空闲连表中没有合适的空间分配 */
    mem_block_t *new_block = (mem_block_t*)alloc_pages(8);
    if (!new_block) {
        return NULL;
    }

    new_block->size = size;
    new_block->next = NULL;
    new_block->prev = NULL;
    new_block->used = USED;
    new_block->is_head = 1;

    mem_block_t *free_block = (mem_block_t*)((uint)new_block + size);
    free_block->size = 8 * PAGE_SIZE - size;
    free_block->next = prev->next;
    prev->next->prev = free_block;
    free_block->prev = prev;
    prev->next = free_block;
    free_block->used = FREE;
    free_block->is_head = 0;

    return (void*)((uchar*)new_block + MEM_HSIZE);
}

/* 释放内核申请的内存 */
void kfree(void* address)
{
    if (address <= 0) {
        return;
    }

    /* 获取头部信息 */
    mem_block_t *block = (mem_block_t*)((uint)address - MEM_HSIZE);
    /* 插入到空闲链表的头部 */
    block->next = free_list;
    free_list->prev = block;
    block->prev = NULL;
    block->used = FREE;
    free_list = block;

    uchar flag = 0;
    mem_block_t *prev = NULL;
    /* 尝试合并相邻的空闲块 */
    mem_block_t *current = free_list;
    while(current) {
        flag = 0;
        mem_block_t *next_block = (mem_block_t*)((uint)current + current->size);
        while (next_block->used == FREE) {
            current->size += next_block->size;
            /* 从空闲链表中删除被合并的节点 */
            current->next = next_block->next;
            next_block->next->prev = current;
            /* 清空，防止后面申请到内存在同一位置，标志位冲突 */
            next_block->used = 0;

            next_block = (mem_block_t*)((uint)current + current->size);
            /* 8页内存的头节点 */
            if (next_block->is_head == 1) {
                flag = 1;
                current = next_block;
                break;
            }
        }
        
        if (flag) {
            continue;
        }

        current = current->next;

        /* 跳过已经被合并的节点, 这里要清空current的next和next，不然
         * 下次申请到这些内存的时候会找到下一个节点，导致分配错误，
         * 下面的释放大块内存也是同理 */
        while (current && current->used == 0) {
            prev = current;
            current = current->next;
            prev->next = NULL;
            prev->prev = NULL;
        }
    }
    
    /* 查找是否有大块，如果有就直接释放 */
    current = free_list;
    mem_block_t *next = current->next;
    while(current) {
        next = current->next;
        if (current->size == 8 * PAGE_SIZE) {
            current->size = 0;
            free_pages((uint)current, 8);
        }

        current = next;
    }
}
