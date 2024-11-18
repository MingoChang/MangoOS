/*
 * st Mango
 *  HostName 116.62.136.89
 *      Port 22
 *          User root
 *              IdentityFile ~/.ssh/Mango.key
 * ==================================
 * auther: Mingo
 * date: 2024-11-16
 * ==================================
 */
#ifndef __SCHED_H__
#define __SCHED_H__

#include "type.h"
#include "task.h"

void switch_to(task_t *prev, task_t *next);
void __switch_to(task_t *prev, task_t *next);
void schedule();

#endif
