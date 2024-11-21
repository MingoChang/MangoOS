/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-12
 * ==================================
 */
#include "../include/time.h"
#include "../include/arch.h"
#include "../include/irq.h"
#include "../include/task.h"

/* 系统滴答 */
static uint sys_tick;

/* 初始化8253定时器芯片 */
void init_8253()
{
    /* 8253芯片1秒就有1193182个节拍 */
    /* 1000 / 10是1秒有多少个10毫秒，控制系统滴答为10毫秒 */
    uint reload_count = 1193182 / (1000 / 10);

    outb(0x43, (0 << 6) | (3 << 4) | (3 << 1));
    outb(0x40, reload_count & 0xFF);
    outb(0x40, (reload_count >> 8) & 0xFF);

    /* 安装系统定时中断函数 */
    irq_install(0x20, (irq_handler_t)exception_handler_time);
    enable_8259(0x20);
}

void time_init()
{
    sys_tick = 0;
    init_8253();
}

void do_handler_time(exception_frame_t* frame)
{
    sys_tick++;
    pic_done(0x20);
    task_tick();
}

