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

queue_t task_queue;
queue_t ready_task_queue;
queue_t sleep_task_queue;

task_t* current;

void init_task_entry()
{
    int count = 0;

    char data[51];

    int fd = sys_open("/boot/example", O_RDONLY);
    if (fd < 0) {
        __asm__ __volatile__("hlt");
    }

    int err = sys_read(fd, data, 50);
    if (err < 0) {
        kprintf("read file err!:%d\n", err);
    }

    sys_close(fd);

    while(1) {
        kprintf("in %s thread: %d\n", current->name, count++);
        sys_sleep(1000);
    }
}

void kernel_init(void)
{
    arch_init();
    time_init();
    log_init();
    mem_init();
    fs_init();

    /* 初始化进程队列和可运行队列 */
    queue_init(&task_queue);
    queue_init(&ready_task_queue);
    queue_init(&sleep_task_queue);

    /* 就是本进程，在切换出去的时候设置 entry。这边先设置为0 */
    task_init("idle task", 0);
    task_init("init task", (uint)init_task_entry);

    current = queue_data(queue_first(&ready_task_queue), task_t, rq);

    /* 开中断 */
    sti();

    /* 作为空闲进程 */
    while(1) {
        __asm__ __volatile__("hlt");
    }
}
