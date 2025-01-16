/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-16
 * ==================================
 */
#include "../include/sched.h"
#include "../include/task.h"
#include "../include/queue.h"
#include "../include/irq.h"
#include "../include/log.h"

extern task_t* idle_task;
extern task_t* current;
extern queue_t ready_task_queue;

static task_t* pick_next_task()
{
    uint eflags = enter_critical();
    queue_t *next = current->rq.next;

    /* 当前任务已经不在运行队列，next是取不到下一个可运行进程的 */
    if (current->state != TASK_RUNNING) {
        next = queue_first(&ready_task_queue);
    }

    /* 队列头并不存储内容 */
    if (next == &ready_task_queue) {
        next = next->next;
    }

    /* 运行队列已经没有进程，执行空闲进程 */
    if (next == &ready_task_queue) {
        next = &idle_task->rq;
    }
    leave_critical(eflags);

    return queue_data(next, task_t, rq);
}

/* 保存当前页表基地址 */
void save_cr3(task_t *task) {
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(task->cr3));
}

/* 加载新的页表 */
void load_cr3(task_t *task) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(task->cr3));
}

void load_tr(int selector)
{
   __asm__ __volatile__("ltr %0" :: "r"(selector));
}

void switch_mm(task_t *prev, task_t *next) {
    if (prev != next) {
        save_cr3(prev);
        load_cr3(next);
    }
}

void switch_tss(task_t *prev, task_t *next)
{
    if (prev != next) {
        /* 保存当前进程的TSS */
        set_tss_desc(&next->tss, TSS_SELECTOR >> 3);

        load_tr(TSS_SELECTOR);
    }
}

void schedule()
{
    task_t *next;

    next = pick_next_task();
    switch_tss(current, next);
	switch_mm(current, next);
    switch_to(current, next);
}

void switch_to(task_t *prev, task_t *next) 
{
    if (prev != next) { 
        __switch_to(prev, next);
    }
}

/* 调试发现，编译器会自动在函数的开头push %ebx，同时
 * 在函数结尾 增加pop %ebx和ret指令,所以这边不需要增
 * 加ret指令，增加的话就会跳过pop %ebx指令，导致返回
 * 地址不对，系统异常 
 * current放置到这边进行切换，保证在切换进程内核栈后
 * pop出来的current切实执行当前进程本身。如果在改函
 * 返回之后再赋值current = next的话是不对的，因为已
 * 完成了进程切换，此时的next是下一个进程环境中的变
 * 量。还有一种方式是类似Linux的实现，直接把进程结构
 * 体放置在内核栈的开始位置，每个进程都有自己的内核
 * 栈，这样让current指向内核栈的其实位置，那么永远都
 * 是执行当前进程的结构体，从而正确引用当前进程 */
void __switch_to(task_t *prev, task_t *next)
{
    __asm__ __volatile__( 
        "push %%ebp\n\t" 
        "pushfl\n\t" 
        "push %%eax\n\t" 
        "push %%ebx\n\t"
        "push %%ecx\n\t"
        "push %%edx\n\t"
        "push %%esi\n\t"
        "push %%edi\n\t"
        "push %3\n\t"
        "mov %%esp, %0\n\t"
        "mov %2, %%esp\n\t"
        "pop %1\n\t"
        "pop %%edi\n\t"
        "pop %%esi\n\t"
        "pop %%edx\n\t"
        "pop %%ecx\n\t"
        "pop %%ebx\n\t"
        "pop %%eax\n\t"
        "popfl\n\t"
        "pop %%ebp\n\t"
       :"=m"(prev->esp), "=m"(current)
       :"m"(next->esp), "m"(current)
       :"memory");
    return;
}
