/*
 * ==================================
 * auther: Mingo
 * date: 2024-10-25
 * ==================================
 */
#include "../include/init.h"
#include "../include/arch.h"
#include "../include/time.h"
#include "../include/log.h"
#include "../include/task.h"
#include "../include/sched.h"

task_t idle_task, init_task;
task_t* current = &idle_task;

/* 第一个进程的栈空间 */
static uint init_stack[1024];

void init_task_entry()
{
    int count = 0;
    while(1) {
        kprintf("in first task thread: %d\n", count++);
    }
}

void kernel_init(void)
{
    arch_init();
    time_init();
    log_init();

    /* 就是本进程，在切换出去的时候这是 entry和esp。这边先设置为0 */
    task_init(&idle_task, 0, 0);
    /* 栈向下增长，所以栈顶在最高位 */
    task_init(&init_task, (uint)init_task_entry, (uint)&init_stack[1024]);
    sti();

    int count = 0;
    while(1) {
        kprintf("in init thread: %d\n", count++);
    }
}
