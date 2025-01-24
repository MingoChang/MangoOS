/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-18
 * ==================================
 */
#ifndef __STRING_H__
#define __STRING_H__

#include "../include/type.h"

void memset(void *dest, uchar c, int size);
void memcpy(void *dest, const void *src, int size);
int memcmp(const void *first, const void *second, int size);
void strcpy(char *dest, const char *src);
int strlen(const char *str);
char *strrchr(const char *str, int c);

#endif
