/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-18
 * ==================================
 */
#include "../include/queue.h"

/* 在链表头部插入节点 */
void queue_insert_head(queue_t *head, queue_t* q)
{
    q->next = head->next;
    q->prev = head;

    head->next->prev = q;
    head->next = q;
}

/* 在链表的尾部插入节点 */
void queue_insert_tail(queue_t *head, queue_t *q)
{
    q->next = head;
    q->prev = head->prev;

    head->prev->next = q;
    head->prev = q;
}

/* 删除节点 */
void queue_remove(queue_t *q)
{
    q->prev->next = q->next;
    q->next->prev = q->prev;

    /* 重新成为独立节点 */
    q->next = q;
    q->prev = q;
}
