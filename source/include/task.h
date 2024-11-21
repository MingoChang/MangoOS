/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-15
 * ==================================
 */
#ifndef __TASK_H__
#define __TASK_H__

#include "type.h"
#include "arch.h"
#include "queue.h"

#define TASK_SLICE 10

typedef struct _task_t
{
    enum {
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_WAITING,
        TASK_ZOMBIE
    }state;

    int pid;
    int slice;  /* 时间片 */
    uint sleep_ticks; /* 睡眠时间 */
    uint eip;   /* 初始任务入口 */
    uint esp;   /* 初始栈顶指针 */
    queue_t q;  /* 进程队列节点 */
    queue_t rq;  /* 运行进程队列节点 */

    uint stack[1024]; /* 进程内核栈空间4K */
}task_t;


int task_init(task_t* task, uint entry);
void task_tick();
void task_yield();
void sys_sleep(uint ms);

#endif
