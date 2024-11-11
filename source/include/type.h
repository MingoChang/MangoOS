/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-07
 * ==================================
 */
#ifndef __TYPE_H__
#define __TYPE_H__

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#define GDT_TABLE_SIZE 128          /* GDT表项数 */
#define IDT_TABLE_SIZE 256          /* IDT表项数 */
#define KERNEL_SELECTOR_CS  1 << 3  /* 内核代码段选择子 */
#define KERNEL_SELECTOR_DS  2 << 3  /* 内核数据段选择子 */

#endif
