/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-08
 * ==================================
 */
#include "../include/arch.h"
#include "../include/irq.h"

static gdt_desc_t gdt_table[GDT_TABLE_SIZE];
static gate_desc_t idt_table[IDT_TABLE_SIZE];

void set_gdt_desc(ushort index, uint base, uint limit, ushort attribute)
{
    gdt_desc_t* desc = gdt_table + index; /* 找到索引对应的GDT项 */

    /* 段界限总共就20位，要表示32位的地址，需要设置属性中的G位，来改变段界限的颗粒度 */
    if (limit > 0xfffff) {
        attribute |= 0x8000;
        limit /= 0x1000; /* 颗粒度为4K */
    }

    desc->llimit = limit & 0xffff;
    desc->lbase = base & 0xffff;
    desc->mbase = (base >> 16) & 0xff;
    desc->attribute = attribute | (((limit >> 16) & 0xf) << 8);
    desc->hbase = (base >> 24) & 0xff;
}

void set_gate_desc(ushort index, ushort selector, uint offset,ushort attribute)
{
    gate_desc_t* desc = idt_table + index;
    
    desc->loffset = offset & 0xffff;
    desc->selector = selector;
    desc->attribute = attribute;
    desc->hoffset = (offset >> 16) & 0xffff;
}

static void init_gdt()
{
    /* 全部先初始化为空 */
    for (int i=0; i<GDT_TABLE_SIZE; i++) {
        set_gdt_desc(i, 0, 0, 0);
    }

    /* 代码段, 可执行，可读，段存在，特权级0 */
    set_gdt_desc(KERNEL_SELECTOR_CS >> 3, 0x0, 0xffffffff, (1 << 7) | (0 << 5) |
          (1 << 4) | 0x0A | (1 << 14 ));
    /* 数据段, 可写，可读，段存在，特权级0 */
    set_gdt_desc(KERNEL_SELECTOR_DS >> 3, 0x0, 0xffffffff, (1 << 7) | (0 << 5) |
            (1 << 4) | 0x2 | (1 << 14));

    load_gdt((uint)gdt_table, sizeof(gdt_table));
    reload_segments();
}

static void init_8259()
{
    /* 边沿触发，级联方式 */
    outb(0x20, (1 << 4) | 1);
    /* 起始中断序号0x20 */
    outb(0x21, 0x20);
    /* 有从片 */
    outb(0x21, 1 << 2);
    /* 不使用缓冲，不自动结束， 8086模式 */
    outb(0x21, 1);
    
    /* 初始化从片 */
    /* 边沿触发，级联方式 */
    outb(0xa0, (1 << 4) | 1);
    /* 起始中断号 */
    outb(0xa1, 0x28);
    /* 无从片，连接主片 */
    outb(0xa1, 1 << 1);
    /* 不使用缓冲，不自动结束， 8086模式 */
    outb(0xa1, 1);

    /* 只允许来自从片的中断 */
    outb(0x21, 0xff & ~(1 << 2));
    outb(0xa1, 0xff);
}

static void init_idt()
{
    /* 全部初始化为默认处理函数 */
    for (int i=0; i<IDT_TABLE_SIZE; i++) {
        set_gate_desc(i, KERNEL_SELECTOR_CS, (uint)exception_handler_default, (1 << 15) | (0 << 13) |
                (0xE << 8));
    }

    irq_install(0, (irq_handler_t)exception_handler_divide);
    irq_install(1, (irq_handler_t)exception_handler_debug);
    irq_install(2, (irq_handler_t)exception_handler_nmi);
    irq_install(3, (irq_handler_t)exception_handler_bp);
    irq_install(4, (irq_handler_t)exception_handler_of);
    irq_install(5, (irq_handler_t)exception_handler_br);
    irq_install(6, (irq_handler_t)exception_handler_io);
    irq_install(7, (irq_handler_t)exception_handler_du);
    irq_install(8, (irq_handler_t)exception_handler_df);
    irq_install(10, (irq_handler_t)exception_handler_it);
    irq_install(11, (irq_handler_t)exception_handler_snp);
    irq_install(12, (irq_handler_t)exception_handler_ssf);
    irq_install(13, (irq_handler_t)exception_handler_gp);
    irq_install(14, (irq_handler_t)exception_handler_pf);
    irq_install(16, (irq_handler_t)exception_handler_fe);
    irq_install(17, (irq_handler_t)exception_handler_ac);
    irq_install(18, (irq_handler_t)exception_handler_mc);
    irq_install(19, (irq_handler_t)exception_handler_simd);
    irq_install(20, (irq_handler_t)exception_handler_virtual);
    irq_install(21, (irq_handler_t)exception_handler_control);

    load_idt((uint)idt_table, sizeof(idt_table));

    /* 初始化8259芯片 */
    init_8259();
}

/* cpu等硬件初始化 */
void arch_init()
{
    init_gdt();
    init_idt();
}
