/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-13
 * ==================================
 */
#include <stdarg.h>
#include "../include/log.h"
#include "../include/irq.h"
#include "../include/ipc.h"

static volatile ushort* vga;
static mutex_t mutex;

void log_init()
{
    mutex_init(&mutex);
    /* 0x5a0为进入内核后屏幕字符的位置 */
    vga = (ushort*)(0xB8000 + 0x500);
}

static void print_integer(uint value, int base) {
    char buffer[32];
    int i = 31;

    /* 数字是0的时候，直接输出字符‘0’ */
    if (value == 0) {
		*vga = 0x07 << 8 | '0'; 
		vga++;
        return;
    }

    /* 是负数的时候增加一个‘-’字符 */
    if (value < 0 && base == 10) {
		*vga = 0x07 << 8 | '-'; 
		vga++;
        value = -value;
    }

    /* 进制的低位到高位，逆序存储在buffer中 */
    while (value != 0 && i >= 0) {
        int digit = value % base;
        buffer[i--] = "0123456789abcdef"[digit];
        value /= base;
    }

    /* 从当前i+1位置正序开始输出 */
    i++;
    while (i <= 31) {
		*vga = 0x07 << 8 | buffer[i++]; 
		vga++;
    }
}

void kprintf(const char *format, ...) 
{
    va_list args;
    va_start(args, format);

    mutex_lock(&mutex);
    while (*format != '\0') {
        /* 处理格式化内容，目前仅支持数字，16进制，字符串 */
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    uint value = va_arg(args, uint);
                    print_integer(value, 10);
                    break;
                }
                case 'x': {
                    uint value = va_arg(args, uint);
                    print_integer(value, 16);
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char *);
                    while (*str != '\0') {
						*vga = 0x07 << 8 | *str++;
						vga++;
                    }
                    break;
                }
                default:
					*vga = 0x07 << 8 | *format;
					vga++;
            }
        } else {
            if (*format == '\n') {
                /* 屏幕当前行还剩下多少字节到屏幕尾 */
                int left = 160 - (((int)vga - 0xB8000) % 160);
                vga+= left / 2;
                format++;
                continue;
            }
            /* 字符直接输出 */
			*vga = 0x07 << 8 | *format;
			vga++;
        }
        format++;

        /* 输出满一页屏幕了，清空屏幕从头开始显示 */
        if (((int)vga - 0xB8000) >= (160 * 25)) {
            format--; /* 清屏后重新显示当前字符 */
            vga = (ushort*)0xB8000;
            for (int i=0; i<80 * 25; i++) {
                vga[i] = ' ';
            }
        }
    }

    mutex_unlock(&mutex);

    va_end(args);
}
