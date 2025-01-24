/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-18
 * ==================================
 */
#include "./string.h"

void memset(void *dest, uchar c, int size)
{
    if ((dest == NULL) || (size <= 0)) {
        return;
    }

    uchar *p = (uchar*)dest;
    while(size--) {
        *p++ = c;
    }
}

void memcpy(void *dest, const void *src, int size)
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

int memcmp(const void *first, const void *second, int size)
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

    return 0;
}

void strcpy(char* dest, const char* src)
{
    if ((dest == NULL) || (src == NULL)) {
        return;
    }

    while(*src != '\0') {
        *dest++ = *src++;
    }
    
    *dest = '\0';
}

int strlen(const char* str)
{
    const char* p = str;

    int len = 0;
    while(*p++) {
        len++;
    }

    return len;
}

char *strrchr(const char *str, int c)
{
    char *last_occurrence = NULL;

    /* 遍历字符串直到结束 */
    while (*str) {
        if (*str == (char)c) {
			/* 更新最后一次出现的指针 */
            last_occurrence = (char *)str; 
        }
        str++;
    }

    /* 检查末尾的 null 字符是否匹配 */
    if (c == '\0') {
        return (char *)str;
    }

    return last_occurrence;
}

