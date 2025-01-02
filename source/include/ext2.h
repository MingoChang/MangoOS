/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-9
 * ==================================
 */
#ifndef __EXT2_H__
#define __EXT2_H__

#include "type.h"
#include "fs.h"

#define S_ISDIR(m)  (((m) & 0170000) == 0040000)
#define S_ISREG(m)  (((m) & 0170000) == 0100000)

#pragma pack(1)

struct _super_block_t;

typedef struct _inode_t
{
    ushort i_mode;      /* 文件类型及访问权限 */
    ushort i_blocks;    /* 文件占用的块数 */
    ushort i_uid;       /* 文件所属的用户id */
    ushort i_gid;       /* 文件所属的组id */
    uint i_size;        /* 文件大小 */
    uint i_atime;       /* 访问时间 */
    uint i_mtime;       /* 修改时间 */
    uint i_ctime;       /* 创建时间 */
    uint i_block[15];   /* 文件块指针 */
    struct _super_block_t *sb;  /* 超级块 */
    struct _inode_t *parent;
}inode_t;

typedef struct _group_desc_t
{
    uint block_bitmap;  /* 块位图起始块 */
    uint inode_bitmap;  /* inode位图起始块 */
    uint inode_table;   /* inode表起始块的ID */
    ushort free_block;  /* 本组中空闲块数 */
    ushort free_inode;  /* 本组中空闲inode节点 */
    ushort dir_inode;   /* 本组中分配给目录的节点数 */
    ushort block_bitmap_size; /* 块位图占内存大小（字节）*/
    ushort inode_bitmap_size; /* inode位图占内存大小（字节）*/
    uchar *p_block_bitmap;  /* 块位图内存地址 */
    uchar *p_inode_bitmap;  /* inode位图内存地址 */
    int inode_begin;       /* 缓存开始块 */
    inode_t *inode_data;    /* inode表缓存区域 */
}group_desc_t;

typedef struct _super_block_t
{
    char name[MAX_NAME_LEN];      /* 文件系统名 */
    ushort magic;       /* 魔数，标记文件系统的类型 */
    uint inode_count;   /* inode节点数 */
    uint block_count;   /* 总块数 */
    uint used_block;    /* 已经分配的块数 */
    uint free_block;    /* 空闲块数 */
    uint free_inode;    /* 空闲inode节点 */
    ushort inode_size;  /* inode结构大小 */
    ushort block_size;  /* 块大小 */
    uint group_block;   /* 每组块数 */
    uint group_inode;   /* 每组包含的inode节点数 */
    uint group_count;   /* 文件系统包含块组数 */
    fs_t *fs;           /* 文件系统 */
    group_desc_t group_table[0];    /*块组描述符表 */
}super_block_t;

typedef struct _dir_t
{
    uint inode;         /* inode 编号 */
    ushort rec_len;     /* 目录项长度 */
    uchar name_len;     /* 文件名包含的字符数 */
    uchar file_type;    /* 文件类型 */
    char name[];      /* 文件名 */
}dir_t;

#pragma pack()

#endif
