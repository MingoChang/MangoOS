/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-07
 * ==================================
 */
#ifndef __IRQ_H__
#define __IRQ_H__

#include "type.h"

typedef struct _exception_frame_t
{
    uint gs, fs, es, ds;
    uint edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint num, error_code, eip, cs, eflags;
}exception_frame_t;

typedef void (*irq_handler_t)();

void irq_init();
int irq_install(int irq_num, irq_handler_t handler);

void exception_handler_default();
void exception_handler_divide();
void exception_handler_debug();
void exception_handler_nmi();
void exception_handler_bp();
void exception_handler_of();
void exception_handler_br();
void exception_handler_io();
void exception_handler_du();
void exception_handler_df();
void exception_handler_it();
void exception_handler_snp();
void exception_handler_ssf();
void exception_handler_gp();
void exception_handler_pf();
void exception_handler_fe();
void exception_handler_ac();
void exception_handler_mc();
void exception_handler_simd();
void exception_handler_virtual();
void exception_handler_control();

void do_handler_default(exception_frame_t* frame);
void do_handler_divide(exception_frame_t* frame);
void do_handler_debug(exception_frame_t* frame);
void do_handler_nmi(exception_frame_t* frame);
void do_handler_bp(exception_frame_t* frame);
void do_handler_of(exception_frame_t* frame);
void do_handler_br(exception_frame_t* frame);
void do_handler_io(exception_frame_t* frame);
void do_handler_du(exception_frame_t* frame);
void do_handler_df(exception_frame_t* frame);
void do_handler_it(exception_frame_t* frame);
void do_handler_snp(exception_frame_t* frame);
void do_handler_ssf(exception_frame_t* frame);
void do_handler_gp(exception_frame_t* frame);
void do_handler_pf(exception_frame_t* frame);
void do_handler_fe(exception_frame_t* frame);
void do_handler_ac(exception_frame_t* frame);
void do_handler_mc(exception_frame_t* frame);
void do_handler_simd(exception_frame_t* frame);
void do_handler_virtual(exception_frame_t* frame);
void do_handler_control(exception_frame_t* frame);

#endif
