/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-16
 * ==================================
 */
#include "../include/syscall.h"
#include "../include/log.h"
#include "../include/irq.h"
#include "../include/log.h"
#include "../include/task.h"

extern task_t *current;
extern void exception_handler_syscall();

typedef int (*syscall_handler_t)(uint arg0, uint arg1, uint arg2, uint arg3);

int sys_printf(char *fmt, int arg, int len)
{
    char msg[len+1];
    copy_from_user(msg, fmt,  len);
    msg[len] = '\0';

    kprintf(msg, arg);
    return 0;
}

static const syscall_handler_t call_table[] = {
    [SYS_SLEEP] = (syscall_handler_t)sys_sleep,
    [SYS_FORK] = (syscall_handler_t)sys_fork,
    [SYS_EXECVE] = (syscall_handler_t)sys_execve,
    [SYS_PRINT] = (syscall_handler_t)sys_printf,
};

void do_handler_syscall(exception_frame_t *frame)
{
    if (frame->eax < sizeof(call_table) / sizeof(call_table[0])) {

        syscall_handler_t handler = call_table[frame->eax];
        if (handler) {
            int ret = handler(frame->ecx, frame->edx, frame->ebp, frame->edi);
            frame->eax = ret;
            return;
        }
    }

    kprintf("task:%s, Unknown syscall:%d\n", current->name, frame->eax);
    return;
}

void syscall_init()
{
    irq_install(0x80, (irq_handler_t)exception_handler_syscall);
}

