/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-8
 * 注：因为队列queut_t实际上也是用链
 *     表实现的，所以其实可以共用，
 *     但我再链表着增加了一个指定位
 *     置插入的功能，同时为了语义更加
 *     准确，所以决定单独再写，代码
 *     基本相同。
 * ==================================
 */
#ifndef __LIST_H__
#define __LIST_H__

#include "type.h"

typedef struct _list_t
{
    struct _list_t *prev;
    struct _list_t *next;
}list_t;

/* 判断链表是否为空 */
#define list_empty(head) \
    (head == (head)->prev)

/* 初始化元素 */
#define list_init(q) \
        (q)->prev = q; \
        (q)->next = q

/* 获取第一个元素 */
#define list_first(head) \
        (head)->next

/* 获取最后一个元素 */
#define list_last(head) \
        (head)->prev

#define list_each(q, head) \
    for (q = (head)->next; q != (head); q = q->next)

#define list_each_reverse(q, head) \
    for (q = (head)->prev; q != (head); q = q-prev)

/* 获取链表元素所在的节点结构体的起始地址 */
#define list_data(q, type, link) \
    (type*) ((uchar *)q - offsetof(type, link))

void list_insert_head(list_t *head, list_t* q);
void list_insert_tail(list_t *head, list_t *q);
void list_insert(list_t *p, list_t *q);
void list_remove(list_t *q);

#endif
