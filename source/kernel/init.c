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
#include "../include/fs.h"
#include "../include/dev.h"
#include "../include/string.h"
#include "../include/syscall.h"

queue_t task_queue;
queue_t ready_task_queue;
queue_t sleep_task_queue;

task_t* current;

void init_task_entry()
{
    int count = 0;

    sys_execve("/boot/init", NULL, NULL);
    switch_to_user_mode(current->tss.eip, current->tss.esp);

    /* 不会再回到这 */
    return;
}

void kernel_init(void)
{
    arch_init();
    time_init();
    log_init();
    mem_init();
    fs_init();
    syscall_init();

    /* 初始化进程队列和可运行队列 */
    queue_init(&task_queue);
    queue_init(&ready_task_queue);
    queue_init(&sleep_task_queue);

    task_t *task_idle = alloc_task();
    /* 就是本进程，在切换出去的时候设置 entry。这边先设置为0 */
    task_init(task_idle, "idle task", 0);
    task_idle->state = TASK_RUNNING;
    
    task_t *task = alloc_task();
    task_init(task, "init task", (uint)init_task_entry);
    task->state = TASK_RUNNING;

    current = queue_data(queue_first(&ready_task_queue), task_t, rq);

    /* 开中断 */
    sti();

    /* 作为空闲进程 */
    while(1) {
        __asm__ __volatile__("hlt");
    }
}
