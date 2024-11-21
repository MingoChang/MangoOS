/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-21
 * ==================================
 */
#include "../include/ipc.h"
#include "../include/irq.h"
#include "../include/sched.h"

extern task_t *current;
extern queue_t ready_task_queue;

void sem_init(sem_t *sem, int count)
{
    sem->count = count;
    queue_init(&sem->sem_queue);
}

void sem_wait(sem_t *sem)
{
    uint eflags = enter_critical();

    /* 可以拿到信号量，减少计数，返回 */
    if (sem->count > 0) {
        sem->count--;
    } else { /* 没有空余，放入等待队列 */
        queue_remove(&current->rq);
        task_t *task = queue_data(&current->rq, task_t, rq);
        task->state = TASK_WAITING;
        queue_insert_tail(&sem->sem_queue, &current->rq);

        schedule();
    }

    leave_critical(eflags);
}

void sem_notify(sem_t *sem)
{
    uint eflags = enter_critical();

    if (!queue_empty(&sem->sem_queue)) {
        queue_t *q = queue_first(&sem->sem_queue);
        queue_remove(q);
        task_t *task = queue_data(q, task_t, rq);
        task->state = TASK_RUNNING;
        queue_insert_tail(&ready_task_queue, q);

        schedule();
    } else {
        /* notify调用次数多余wait的话，会导致count多于初始值
         * 但我们设计就是成对使用的，这边不做处理 */
        sem->count++;
    }

    leave_critical(eflags);
}

void mutex_init(mutex_t *mutex)
{
    mutex->lock_times = 0;
    mutex->owner = NULL;
    queue_init(&mutex->mutex_queue);
}

void mutex_lock(mutex_t *mutex)
{
    uint eflags = enter_critical();
    if (mutex->lock_times == 0) {
        mutex->lock_times++;
        mutex->owner = current;
    } else if (mutex->owner == current) {
        /* 同个进程多次上锁 */
        mutex->lock_times++;
    } else {
        queue_remove(&current->rq);
        task_t *task = queue_data(&current->rq, task_t, rq);
        task->state = TASK_WAITING;
        queue_insert_tail(&mutex->mutex_queue, &current->rq);

        schedule();
    }

    leave_critical(eflags);
}

void mutex_unlock(mutex_t *mutex)
{
    uint eflags = enter_critical();

    if (mutex->owner == current) {
        if (--mutex->lock_times == 0) {
            /* 释放，设置锁无主人 */
            mutex->owner = NULL;
            
            /* 如果锁的等待队列不为空，则取队列头那个进程，占有锁 */
            if (!queue_empty(&mutex->mutex_queue)) {
                queue_t *q = queue_first(&mutex->mutex_queue);
                queue_remove(q);
                task_t *task = queue_data(q, task_t, rq);
                task->state = TASK_RUNNING;
                queue_insert_tail(&ready_task_queue, q);

                mutex->owner = task;
                mutex->lock_times = 1;

                schedule();
            }
        }
    }

    leave_critical(eflags);
}
