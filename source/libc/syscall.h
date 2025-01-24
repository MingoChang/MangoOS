/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-18
 * ==================================
 */
#ifndef __LIBC_SYSCALL_H__
#define __LIBC_SYSCALL_H__

typedef struct _syscall_args_t {
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
}syscall_args_t;

#define SYS_SLEEP   0
#define SYS_FORK    1
#define SYS_EXECVE  2
#define SYS_PRINT   100

int print_msg(char* fmt, int arg);
int sleep(int ms);
int fork();
int execve(char *name, char *argv[], char *env[]);

#endif
