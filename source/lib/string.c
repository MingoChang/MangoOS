/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-14
 * ==================================
 */
#include "../include/string.h"

void kmemset(void *dest, uchar c, int size)
{
    if ((dest == NULL) || (size <= 0)) {
        return;
    }

    uchar *p = (uchar*)dest;
    while(size--) {
        *p++ = c;
    }
}

void kmemcpy(void *dest, void *src, int size)
{
    if ((dest == NULL) || (src == NULL) || size <= 0) {
        return;
    }

    uchar *s = (uchar*)src;
    uchar *d = (uchar*)dest;

    while(size--) {
        *d++ = *s++;
    }
}

int kmemcmp(void *first, void *second, int size)
{
    if ((first == NULL) || (second == NULL)) {
        return 0;
    }

    uchar *f = (uchar*)first;
    uchar *s = (uchar*)second;

    while(size--) {
        if (*f++ != *s++) {
            return 1;
        }
    }

    return 1;
}

void kstrcpy(char* dest, char* src)
{
    if ((dest == NULL) || (src == NULL)) {
        return;
    }

    while(*src != '\0') {
        *dest++ = *src++;
    }
    
    *dest = '\0';
}

int kstrlen(const char* str)
{
    const char* p = str;

    int len = 0;
    while(*p++) {
        len++;
    }

    return len;
}