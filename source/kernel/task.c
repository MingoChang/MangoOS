/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-15
 * ==================================
 */
#include "../include/task.h"
#include "../include/string.h"

static int pid = 1;
extern task_t init_task;

int task_init(task_t* task, uint entry, uint esp)
{
    if (task == NULL) {
        return 1;
    }

    task->pid = pid++; 
    /* 时间片初始化为3 */
    task->slice = 3;

    uint *pesp = (uint*)esp;
    if (pesp) {
        *(--pesp) = entry; /* __switch_to中的ret会返回到这个地址 */
        *(--pesp) = 0;
        *(--pesp) = (1 << 1) | (1 << 9);
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = (uint)&init_task;
    }
    task->esp = (uint)pesp;

    return 0;
}
