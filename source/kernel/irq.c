/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-10
 * ==================================
 */
#include "../include/irq.h"
#include "../include/arch.h"

static void do_default_handler(exception_frame_t* frame, char *messsage)
{
    __asm__ __volatile("hlt");
}

void do_handler_default(exception_frame_t* frame)
{
    do_default_handler(frame, "unknow exception");
}
void do_handler_divide(exception_frame_t* frame)
{
    do_default_handler(frame, "div zero exception");
}
void do_handler_debug(exception_frame_t* frame)
{
    do_default_handler(frame, "debug exception");
}
void do_handler_nmi(exception_frame_t* frame)
{
    do_default_handler(frame, "nmi exception");
}
void do_handler_bp(exception_frame_t* frame)
{
    do_default_handler(frame, "backpoint exception");
}
void do_handler_of(exception_frame_t* frame)
{
    do_default_handler(frame, "over flow exception");
}
void do_handler_br(exception_frame_t* frame)
{
    do_default_handler(frame, "bound range exception");
}
void do_handler_io(exception_frame_t* frame)
{
    do_default_handler(frame, "invalid opcode exception");
}
void do_handler_du(exception_frame_t* frame)
{
    do_default_handler(frame, "device unabavliable exception");
}
void do_handler_df(exception_frame_t* frame)
{
    do_default_handler(frame, "double fault exception");
}
void do_handler_it(exception_frame_t* frame)
{
    do_default_handler(frame, "invalid tss exception");
}
void do_handler_snp(exception_frame_t* frame)
{
    do_default_handler(frame, "segment not present exception");
}
void do_handler_ssf(exception_frame_t* frame)
{
    do_default_handler(frame, "stack segment fault exception");
}
void do_handler_gp(exception_frame_t* frame)
{
    do_default_handler(frame, "general protection exception");
}
void do_handler_pf(exception_frame_t* frame)
{
    do_default_handler(frame, "page fault exception");
}
void do_handler_fe(exception_frame_t* frame)
{
    do_default_handler(frame, "fpu error exception");
}
void do_handler_ac(exception_frame_t* frame)
{
    do_default_handler(frame, "aligment check exception");
}
void do_handler_mc(exception_frame_t* frame)
{
    do_default_handler(frame, "machine check exception");
}
void do_handler_simd(exception_frame_t* frame)
{
    do_default_handler(frame, "simd exception");
}
void do_handler_virtual(exception_frame_t* frame)
{
    do_default_handler(frame, "virtual exception");
}
void do_handler_control(exception_frame_t* frame)
{
    do_default_handler(frame, "control exception");
}

int irq_install(int irq_num, irq_handler_t handler)
{
    if (irq_num > 128) {
        return -1;
    }
    
    set_gate_desc(irq_num, KERNEL_SELECTOR_CS, (uint)handler, (1 << 15) | (0 << 13) | (0xE << 8));

    return 0;
}
