/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-16
 * ==================================
 */
#include "./syscall.h"
#include "./string.h"

static inline int sys_call(syscall_args_t *args)
{
    int ret;

    __asm__ __volatile__ (
        "mov %[arg3], %%edi\n\t"
        "mov %[arg2], %%ebp\n\t"
        "mov %[arg1], %%edx\n\t"
        "mov %[arg0], %%ecx\n\t"
        "mov %[id], %%eax\n\t"
        "int $0x80\n\t"
        :"=a"(ret)
        :[arg3]"r"(args->arg3), [arg2]"r"(args->arg2), [arg1]"r"(args->arg1), [arg0]"r"(args->arg0), [id]"r"(args->id)
        :"memory");

    return ret;
}

int print_msg(char *fmt, int arg)
{
    syscall_args_t args;

    args.id = SYS_PRINT;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    args.arg2 = strlen(fmt);
    return sys_call(&args);
}

int sleep(int ms)
{
    syscall_args_t args;
    args.id = SYS_SLEEP;
    args.arg0 = ms;
    return sys_call(&args);
}

int fork()
{
    syscall_args_t args;
    args.id = SYS_FORK;
    return sys_call(&args);
}

int execve(char *name, char *argv[], char *env[])
{
    syscall_args_t args;
    args.id = SYS_EXECVE;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    return sys_call(&args);
}
