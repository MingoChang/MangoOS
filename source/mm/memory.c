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
    free_list->size = 8 * PAGE_SIZE - sizeof(mem_block_t);
    free_list->used = FREE;
    free_list->is_head = 1;
    free_list->prev = NULL;
    free_list->next = NULL;

}

/* 分配内核内存，内核虚拟内存==物理内存 */
void* kmalloc(uint size)
{
    /* 最少申请8个字节, 最多8页内存 - sizeof(mem_block_t) */
    if (size < 8 || size > 8 * PAGE_SIZE - sizeof(mem_block_t)) {
        kprintf("The size %d is not allow to alloc, the max alloc size is %d", size, 8 * PAGE_SIZE - sizeof(mem_block_t));
        return NULL;
    }

    mem_block_t *current = free_list;
    mem_block_t *prev = NULL;

    /* 遍历空闲链表寻找合适的内存块 */
    while (current) {
        if (current->size >= size) {
            /* 内存分配了还有多， 多的部分作为一个新的空闲块 */
            if (current->size > size + sizeof(mem_block_t)) {
                mem_block_t *new_block = (mem_block_t*)((uint)current + size + sizeof(mem_block_t));            
                new_block->size = current->size - size - sizeof(mem_block_t);
                new_block->prev = current;
                new_block->next = current->next;
                new_block->used = FREE;
                new_block->is_head = 0;

                current->size = size;
                current->next = new_block;
                current->used = USED;
            }

            /* 当前节点已经分配，从空闲链表中删除 */
            if (current->prev) {
                current->prev->next = current->next;
                current->next->prev = current->prev;
            } else {
                free_list = current->next;
                current->next->prev = NULL;
            }

            return (void*)((uchar*)current + sizeof(mem_block_t));
        }

        prev = current;
        current = current->next;
    }

    /* 现有空闲连表中没有合适的空间分配 */
    mem_block_t *new_block = (mem_block_t*)alloc_pages(8);
    if (!new_block) {
        return NULL;
    }

    new_block->size =  size;
    new_block->next = NULL;
    new_block->prev = NULL;
    new_block->used = USED;
    new_block->is_head = 1;

    mem_block_t *free_block = (mem_block_t*)((uint)new_block + size + sizeof(mem_block_t));
    /* 这边包含两个mem_block_t占的空间，一个是new_block分配出去的，一个是free_block的 */
    free_block->size = 8 * PAGE_SIZE - size - 2 * sizeof(mem_block_t);
    free_block->next = prev->next;
    free_block->prev = prev;
    prev->next = free_block;
    free_block->used = FREE;
    free_block->is_head = 0;

    return (void*)((uchar*)new_block + sizeof(mem_block_t));
}

/* 释放内核申请的内存 */
void kfree(void* address)
{
    if (address == 0) {
        return;
    }

    /* 获取头部信息 */
    mem_block_t *block = (mem_block_t*)((uint)address - sizeof(mem_block_t));
    /* 插入到空闲链表的头部 */
    block->next = free_list;
    block->prev = NULL;
    block->used = FREE;
    free_list = block;

    uchar flag = 0;
    mem_block_t *prev = NULL;
    /* 尝试合并相邻的空闲块 */
    mem_block_t *current = free_list;
    while(current) {
        flag = 0;
        mem_block_t *next_block = (mem_block_t*)((uint)current + current->size + sizeof(mem_block_t));
        while (next_block->used == FREE) {
            current->size += next_block->size + sizeof(mem_block_t);
            /* 从空闲链表中删除被合并的节点 */
            current->next = next_block->next;
            next_block->next->prev = current;
            /* 清空，防止后面申请到内存在同一位置，标志位冲突 */
            next_block->used = 0;

            next_block = (mem_block_t*)((uint)current + current->size + sizeof(mem_block_t));
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
    while(current) {
        prev = current;
        if (current->size + sizeof(mem_block_t) == 8 * PAGE_SIZE) {
            current->size = 0;
            free_pages((uint)current, 8);
        }

        current = current->next;
        prev->next = NULL;
        prev->prev = NULL;
    }
}
