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

typedef struct _task_t
{
    int pid;
    int slice;  /* 时间片 */
    uint eip;   /* 初始任务入口 */
    uint esp;   /* 初始栈顶指针 */
}task_t;

int task_init(task_t* task, uint entry, uint esp);

#endif
