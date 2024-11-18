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

void time_init();
void exception_handler_time();
void do_handler_time(exception_frame_t* frame);

#endif
