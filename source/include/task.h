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
#include "file.h"

#define TASK_SLICE 10
#define TASK_OPEN_MAX_FILES 128

struct _file_t;
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
    char name[32];
    int slice;  /* 时间片 */
    uint cr3;   /* 进程页目录起始地址 */
    uint sleep_ticks; /* 睡眠时间 */
    uint eip;   /* 初始任务入口 */
    uint esp;   /* 初始栈顶指针 */
    queue_t q;  /* 进程队列节点 */
    queue_t rq;  /* 运行进程队列节点 */
    tss_t tss;
    struct _task_t *parent;
    struct _file_t *open_file_table[TASK_OPEN_MAX_FILES];
}task_t;

task_t* alloc_task();
int task_init(task_t* task, const char *name, uint entry);
void task_tick();
void task_yield();
void sys_sleep(uint ms);
int sys_fork();
int task_alloc_fd(struct _file_t *file);
void task_free_fd(int fd);
int sys_execve(char *name, char **argv, char **env);
int copy_to_user(void* to, const void* from, uint size);
int copy_from_user(void* to, const void* from, uint size);
void switch_to_user_mode(uint eip, uint esp);

#endif
