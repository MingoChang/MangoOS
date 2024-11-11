/*
 * ==================================
 * auther: Mingo
 * date: 2024-10-25
 * ==================================
 */
#include "../include/init.h"
#include "../include/arch.h"

void kernel_init(void)
{
    volatile unsigned short* vga = (unsigned short*)0xB8050;
    *vga = 0x07 << 8 | 'B';

    arch_init();

    int i = 3/0;
    while(1) {
    }
}
