/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-5
 * ==================================
 */
#include "../libc/syscall.h"

void user_init(void)
{
    int pid = fork();
    if (pid == 0) {
            int count = 0;
        while (1) {
            print_msg("child in user mode, pid=%d\n", count++);
            sleep(1000);
        }
    } else {
        int count = 0;
        while (1) {
            print_msg("father in user mode, pid=%d\n", count++);
            sleep(1000);
        }
    }
}
