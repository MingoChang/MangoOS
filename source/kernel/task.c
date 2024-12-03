/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-15
 * ==================================
 */
#include "../include/task.h"
#include "../include/string.h"
#include "../include/sched.h"
#include "../include/queue.h"
#include "../include/irq.h"
#include "../include/error.h"
#include "../include/page.h"
#include "../include/memory.h"
#include "../include/string.h"

static int pid = 0;
extern queue_t task_queue;
extern queue_t ready_task_queue;
extern queue_t sleep_task_queue;
extern task_t *current;
task_t *idle_task = NULL;

/* 分配任务结构 */
static task_t* alloc_task()
{
    /* 结构体和内核栈共4K */
    task_t *task = (task_t*)kmalloc(4096);
    return task;
}

/* 释放任务结构 */
static void free_task(task_t *task)
{
    kfree((void*)task);
}

int task_init(const char *name, uint entry)
{
    task_t *task = alloc_task();
    if (task == NULL) {
        return -ENOMEM;
    }

    task->pid = pid++; 
    kstrcpy(task->name, name);   
    /* 时间片初始化为1 */
    task->slice = TASK_SLICE;

    uint *pesp = (uint*)((void*)task + 4096);
    if (pesp) {
        *(--pesp) = entry; /* __switch_to中的ret会返回到这个地址 */
        *(--pesp) = 0;     /* 进程切换的时候__switch_to会自动插入%%ebx，所以还需一个空间来存放 */
        *(--pesp) = 0;
        *(--pesp) = (1 << 1) | (1 << 9);
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = (uint)task;
    }
    task->esp = (uint)pesp;
    task->cr3 = create_pde();
    task->state = TASK_RUNNING;

    /* 空闲进程在调度的地方需要使用 */
    if (entry == 0) {
        idle_task = task;
    }

    queue_insert_tail(&task_queue, &task->q);
    queue_insert_tail(&ready_task_queue, &task->rq);

    return 0;
}

/* 进程睡眠 */
static void task_sleep(task_t *task, uint ticks)
{
    if (ticks == 0) {
        return;
    }

    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;

    uint eflags = enter_critical();
    queue_remove(&task->rq);
    queue_insert_tail(&sleep_task_queue, &task->rq);
    leave_critical(eflags);
}

/* 进程唤醒 */
static void task_wakeup(task_t *task)
{
    uint eflags = enter_critical();
    queue_remove(&task->rq);
    queue_insert_tail(&ready_task_queue, &task->rq);
    leave_critical(eflags);
}

/* 定时器内检测进程时间片是否使用完 */
void task_tick()
{
    if (--current->slice == 0) {
        current->slice = TASK_SLICE;
        /* 将进程移动到运行队列的尾部 */
        queue_remove(&current->rq);
        queue_insert_tail(&ready_task_queue, &current->rq);

        schedule();
    }

    /* 查看睡眠队列，是否有进程该唤醒了 */
    queue_t *q = queue_first(&sleep_task_queue);
    while(q != &sleep_task_queue) {
        /* 本节点的后续节点要先取，因为wakeup会吧节点从睡眠队列中移到就绪队列，则next是就绪队列中的nexit */
        queue_t *next = q->next;
        task_t *task = queue_data(q, task_t, rq);
        if (--task->sleep_ticks == 0) {
            task_wakeup(task);
        }

        q = next;
    }

    schedule();
}

/* 进程主动放弃CPU */
void task_yield()
{
    uint eflags = enter_critical();
    /* 将进程移动到运行队列的尾部 */
    queue_remove(&current->rq);
    queue_insert_tail(&ready_task_queue, &current->rq);

    schedule();
    leave_critical(eflags);
}

/* 睡眠 */
void sys_sleep(uint ms)
{
    task_sleep(current, ms / TASK_SLICE);
    schedule();
}
