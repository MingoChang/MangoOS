/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-5
 * ==================================
 */
#ifndef __FS_H__
#define __FS_H__

#include "type.h"
#include "file.h"
#include "ipc.h"
#include "list.h"

#define MAX_MOUNT 10

#define O_RDONLY    0       // 只读
#define O_WRONLY    1       // 只写
#define O_RDWR      2       // 读写
#define O_APPEND    8       // 追加
#define O_CREAT     64      // 创建文件

struct _fs_t;
struct _file_t;
struct _mutex_t;
struct _super_block_t;

enum {
    FS_EXT2 = 1, /* 从1开始，0用来判断文件系统是否挂在 */
};

/* 文件系统操作接口 */
typedef struct _fs_op_t
{
    int (*mount)(struct _fs_t* fs, int major, int minor);
    void (*umount)(struct _fs_t *fs);
    int (*open)(struct _file_t* file, const char *name);
    int (*read)(struct _file_t* file, char *data, int size);
    int (*write)(struct _file_t *file, const char *data, int size);
    int (*seek)(struct _file_t *file, uint offset);
    int (*sync)(struct _super_block_t *block);
    int (*close)(struct _file_t *file);
    int (*unlink)(struct _file_t* file, const char *name);
}fs_op_t;

/* 文件系统结构 */
typedef struct _fs_t {
    char mount_point[128];       /* 挂载点 */
    uchar type;                 /* 文件系统类型 */
    fs_op_t *op;                /* 文件系统接口 */
    int dev_id;                 /* 所属的设备ID */
    void *data;                 /* 文件系统数据 */
    struct _mutex_t *mutex;     /* 文件操作互斥锁 */
}fs_t;

void fs_init();
int sys_open(const char* name, int mode);
int sys_read(int fd, char* data, int len);
int sys_write(int fd, char* data, int len);
int sys_seek(int fd, int offset);
int sys_close(int fd);

#endif
