/*
 * ==================================
 * auther: Mingo
 * date: 2025-1-2
 * ==================================
 */
#ifndef __ELF_H__
#define __ELF_H__

#include "type.h"

#pragma pack(1)

typedef struct _elf_head_t
{
    uchar e_ident[16]; 	/* 魔数和其他信息 */
    ushort e_type;      /* 文件类型 */
    ushort e_machine;   /* 目标体系结构 */
    uint e_version;   	/* 文件版本 */
    uint e_entry;     	/* 入口地址 */
    uint e_phoff;     	/* 程序头表偏移 */
    uint e_shoff;   	/* 节头表偏移 */
    uint e_flags; 	    /* 特定处理器标志 */
    ushort e_ehsize;    /* ELF 文件头大小 */
    ushort e_phentsize; /* 程序头表条目大小 */
    ushort e_phnum;     /* 程序头表条目数量 */
    ushort e_shentsize; /* 节头表条目大小 */
    ushort e_shnum;     /* 节头表条目数量 */
    ushort e_shstrndx;  /* 节头字符串表索引 */ 
}elf_head_t;

typedef struct _elf_phead_t
{
    uint p_type;   /* 段的类型 */
    uint p_offset; /* 段在文件中的偏移 */
    uint p_vaddr;  /* 段的虚拟地址 */
    uint p_paddr;  /* 段的物理地址（通常未使用） */
    uint p_filesz; /* 段在文件中的大小 */
    uint p_memsz;  /* 段在内存中的大小 */
    uint p_flags;  /* 段标志 */
    uint p_align;  /* 段对齐 */
}elf_phead_t;

#pragma pack()

#endif
