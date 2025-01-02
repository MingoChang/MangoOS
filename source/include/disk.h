/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-3
 * ==================================
 */
#ifndef __DISK_H__
#define __DISK_H__

#include "type.h"

typedef struct _part_info_t
{
	char name[32];			/* 分区名称 */
	int type;				/* 文件系统类型 */
	uint begin_sec;			/* 起始扇区 */
	uint count;				/* 扇区数 */
}part_info_t;

typedef struct _part_item_t
{
    uchar active;           /* 是否活动 */
    uchar header;           /* 起始header */
    ushort begin_sec:6;     /* 起始扇区 */
    ushort begin_cylin:10;  /* 起始柱面 */
    uchar fs_type;          /* 文件系统类型 */
    uchar end_header;       /* 结束header */
    ushort end_sec:6;       /* 结束扇区 */
    ushort end_cylin:10;    /* 结束柱面 */
    uint relative_sec;      /* 相对起始扇区 */
    uint count;             /* 总扇区数 */
}part_item_t;

void disk_init();

#endif
