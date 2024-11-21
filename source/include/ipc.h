/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-21
 * ==================================
 */
#ifndef __IPC_H__
#define __IPC_H__

#include "type.h"
#include "queue.h"
#include "task.h"

typedef struct _sem_t
{
    int count;
    queue_t sem_queue;
}sem_t;

typedef struct _mutex_t
{
    int lock_times;
    task_t *owner;
    queue_t mutex_queue;
}mutex_t;

void sem_init(sem_t *sem, int count);
void mutex_init(mutex_t *mutex);

void sem_wait(sem_t *sem);
void sem_notify(sem_t *sem);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif
