/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-14
 * ==================================
 */
#ifndef __STRING_H__
#define __STRING_H__

#include "type.h"

void kmemset(void *dest, uchar c, int size);
void kmemcpy(void *dest, const void *src, int size);
int kmemcmp(const void *first, const void *second, int size);
void kstrcpy(char *dest, const char *src);
int kstrlen(const char *str);
char *kstrrchr(const char *str, int c);

#endif
