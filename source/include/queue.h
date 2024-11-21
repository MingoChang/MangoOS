/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-18
 * ==================================
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "type.h"

typedef struct _queue_t
{
    struct _queue_t *prev;
    struct _queue_t *next;
}queue_t;

/* 判断队列是否为空 */
#define queue_empty(head) \
    (head == (head)->prev)

/* 初始化元素 */
#define queue_init(q) \
        (q)->prev = q; \
        (q)->next = q

/* 获取第一个元素 */
#define queue_first(head) \
        (head)->next

/* 获取最后一个元素 */
#define queue_last(head) \
        (head)->prev

#define queue_each(q, head) \
    for (q = (head)->next; q != (head); q = q->next)

#define queue_each_reverse(q, head) \
    for (q = (head)->prev; q != (head); q = q-prev)

/* 获取队列元素所在的节点结构体的起始地址 */
#define queue_data(q, type, link) \
    (type*) ((uchar *)q - offsetof(type, link))

void queue_insert_head(queue_t *head, queue_t *q);
void queue_insert_tail(queue_t *head, queue_t *q);
void queue_remove(queue_t *q);

#endif
