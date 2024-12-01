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
#include "../include/queue.h"
#include "../include/memory.h"

queue_t task_queue;
queue_t ready_task_queue;
queue_t sleep_task_queue;

task_t idle_task, init_task;
task_t* current;

void init_task_entry()
{
    int count = 0;
    while(1) {
        kprintf("in first task thread: %d\n", count++);
        sys_sleep(1000);
    }
}

void kernel_init(void)
{
    arch_init();
    time_init();
    log_init();
    mem_init();

    /* 初始化进程队列和可运行队列 */
    queue_init(&task_queue);
    queue_init(&ready_task_queue);
    queue_init(&sleep_task_queue);

    /* 就是本进程，在切换出去的时候设置 entry。这边先设置为0 */
    task_init(&idle_task, 0);
    task_init(&init_task, (uint)init_task_entry);

    current = queue_data(queue_first(&ready_task_queue), task_t, rq);

    /* 开中断 */
    sti();

    /* 作为空闲进程 */
    while(1) {
        __asm__ __volatile__("hlt");
    }
}
