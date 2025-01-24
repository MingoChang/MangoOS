/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-16
 * ==================================
 */
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define SYS_SLEEP   0
#define SYS_FORK    1
#define SYS_EXECVE  2
#define SYS_PRINT   100

void syscall_init();

#endif
