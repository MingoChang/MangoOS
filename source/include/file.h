/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-9
 * ==================================
 */
#ifndef __FILE_H__
#define __FILE_H__

#include "type.h"
#include "fs.h"

#define MAX_OPEN_FILES 2048
#define MAX_NAME_LEN 64

struct _fs_t;
typedef struct _file_t
{
    char type;          /* 文件类型 */
    int pos;            /* 当前读写位置 */
    int mode;           /* 读写模式 */
    int start_block;    /* 起始块 */
    int cur_block;      /* 当前块 */
    int ino;            /* inode ID */
    struct _inode_t *inode;     /* 文件对应的inode节点 */
    int ref;            /* 引用次数 */
    struct _fs_t *fs;   /* 文件所属文件系统 */
}file_t;

void file_table_init();
file_t* alloc_file();
void free_file(file_t *file);
void file_inc_ref(file_t *file);

#endif
