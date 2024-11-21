/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-07
 * ==================================
 */
#ifndef __SYS_H__
#define __SYS_H__

#include "type.h"

#pragma pack(1) /* 结构体按1字节对齐 */
typedef struct _gdt_desc_t
{
    ushort llimit;  /* 段界限的低16位 */
    ushort lbase;   /* 基址低16位 */
    uchar mbase;    /* 基址中8位 */
    ushort attribute;   /* 属性和段界限高4位 */
    uchar hbase;    /* 基址高8位 */
}gdt_desc_t;

typedef struct _gate_desc_t
{
    ushort loffset;     /* 偏移低16位 */
    ushort selector;    /* 选择子 */
    ushort attribute;   /* 属性 */
    ushort hoffset;     /* 偏移高16位 */
}gate_desc_t;

typedef struct _GDTR 
{
    ushort limit;
    uint base;
}GDTR;

typedef struct _IDTR 
{
    ushort limit; 
    uint base; 
}IDTR;

#pragma pack() /* 结构体恢复默认对其方式 */

void arch_init();

static inline uchar inb(ushort port) {
	uchar rv;
	__asm__ __volatile__("inb %[p], %[v]" : [v]"=a" (rv) : [p]"d"(port));
	return rv;
}

static inline ushort inw(ushort port) {
	ushort rv;
	__asm__ __volatile__("in %1, %0" : "=a" (rv) : "dN" (port));
	return rv;
}

static inline void outb(ushort port, uchar data) {
	__asm__ __volatile__("outb %[v], %[p]" : : [p]"d" (port), [v]"a" (data));
}

static inline void outw(ushort port, ushort data) {
	__asm__ __volatile__("out %[v], %[p]" : : [p]"d" (port), [v]"a" (data));
}

static inline void cli() {
	__asm__ __volatile__("cli");
}

static inline void sti() {
	__asm__ __volatile__("sti");
}

static inline void load_gdt(uint base, ushort limit)
{
    GDTR gdtr;
    gdtr.limit = limit - 1;
    gdtr.base = base;

    __asm__ __volatile__("lgdt %0" :: "m"(gdtr));
}

static inline void load_idt(uint base, ushort limit)
{
    IDTR idtr;
    idtr.limit = limit - 1;
    idtr.base = base;

    __asm__ __volatile__("lidt %0" :: "m"(idtr));
}

static inline uint read_eflags()
{
    uint eflags;
    __asm__ __volatile__(
        "pushfl\n\t"
        "pop %0\n\t"
        :"=r"(eflags)
        ::"memory"
        );

    return eflags;
}

static inline void write_eflags(uint eflags)
{
    __asm__ __volatile__(
        "push %0\n\t"
        "popf\n\t"
        ::"r"(eflags)
        :"memory"
        );
}

static inline void reload_segments() 
{
    // 重新加载数据段寄存器
    __asm__ __volatile__ (
        "mov %0, %%ds\n\t"
        "mov %0, %%es\n\t"
        "mov %0, %%fs\n\t"
        "mov %0, %%gs\n\t"
        "mov %0, %%ss\n\t"
        : : "r"(KERNEL_SELECTOR_DS) : "memory"
    );

    // 使用跳转重新加载代码段寄存器
    __asm__ __volatile__ (
        "ljmp %0, $reload_cs\n\t"
        "reload_cs:\n\t"
        : : "i"(KERNEL_SELECTOR_CS)
    );
}

void set_gdt_desc(ushort index, uint base, uint limit, ushort attribute);
void set_gate_desc(ushort index, ushort selector, uint offset, ushort attribute);

#endif
