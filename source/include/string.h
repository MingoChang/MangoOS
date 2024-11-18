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
void kmemcpy(void *dest, void *src, int size);

#endif
