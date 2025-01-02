/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-8
 * ==================================
 */
#include "../include/list.h"

/* 在链表头部插入节点 */
void list_insert_head(list_t *head, list_t* q)
{
    q->next = head->next;
    q->prev = head;

    head->next->prev = q;
    head->next = q;
}

/* 在链表的尾部插入节点 */
void list_insert_tail(list_t *head, list_t *q)
{
    q->next = head;
    q->prev = head->prev;

    head->prev->next = q;
    head->prev = q;
}

/* 在指定位置插入节点 */
void list_insert(list_t *p, list_t *q)
{
    q->next = p->next;
    q->prev = p;

    p->next = q;
    p->next->prev = q;
}

/* 删除节点 */
void list_remove(list_t *q)
{
    q->prev->next = q->next;
    q->next->prev = q->prev;

    /* 重新成为独立节点 */
    q->next = q;
    q->prev = q;
}
