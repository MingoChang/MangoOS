/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-12
 * ==================================
 */
#ifndef __TIME_H__
#define __TIME_H__

#include "type.h"
#include "irq.h"

typedef struct _time_t
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_day;
    int tm_mon;
    int tm_year;
}time_t;

void time_init();

#endif
