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
#include "../include/log.h"

/* 系统滴答 */
uint sys_tick;
uint timestamp;

#define MINUTE 60
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
#define YEAR (365 * DAY)

extern void exception_handler_time();
extern task_t* current;

static int month[12] = {
    0,
    DAY * (31),
    DAY * (31 + 29),
    DAY * (31 + 29 + 31),
    DAY * (31 + 29 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    DAY * (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
};

static uint kernel_mktime(time_t *tm)
{
    uint res;
    int year;

    year = tm->tm_year - 1970;
    /* 闰年处理，有几个能被4整除的年份 */
    res = YEAR * year + DAY * ((year + 1) / 4);
    res += month[tm->tm_mon];

    /* 月份从0开始，>1的意思是到达3月份，要判断当前年份不是闰年, 前面+=month已经档闰年 */
    if (tm->tm_mon > 1 && ((year + 2) % 4)) {
        res -= DAY;
    }

    res += DAY * (tm->tm_day -1);
    res += HOUR * tm->tm_hour;
    res += MINUTE * tm->tm_min;
    res += tm->tm_sec;
    
    return res;
}

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

/* 获取RTC寄存器数据 */
static uchar get_rtc_reg(uchar reg)
{
    outb(0x70, reg);
    return inb(0x71);
}

/* 获取RTC时间 */
static void get_rtc_time(time_t *time)
{
    uint second, minute, hour, day, month, year;

    second = get_rtc_reg(0x00);
    minute = get_rtc_reg(0x02);
    hour = get_rtc_reg(0x04);
    day = get_rtc_reg(0x07);
    month = get_rtc_reg(0x08);
    year = get_rtc_reg(0x09);

    /* 将取到的BDC格式的内容转化成十进制 */
    time->tm_sec = (second & 0x0F) + ((second >> 4) * 10);
    time->tm_min = (minute & 0x0F) + ((minute >> 4) * 10);
    time->tm_hour = (hour & 0x0F) + ((hour >> 4) * 10);
    time->tm_day = (day & 0x0F) + ((day >> 4) * 10);
    time->tm_mon = (month & 0x0F) + ((month >> 4) * 10) - 1;
    time->tm_year = (year & 0x0F) + ((year >> 4) * 10) + 2000;
}

void time_init()
{
    sys_tick = 0;
    init_8253();

    time_t tm;
    get_rtc_time(&tm);

    timestamp = kernel_mktime(&tm);
}

static void update_timestamp()
{
    static uchar loop = 0;
    loop++;
    if (loop == 100) {
        timestamp++;
        loop = 0;
    }
}

void do_handler_time(exception_frame_t* frame)
{
    sys_tick++;
    pic_done(0x20);
    
    task_tick();
    update_timestamp();
}

