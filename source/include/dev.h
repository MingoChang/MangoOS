/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-3
 * ==================================
 */
#ifndef __DEV_H__
#define __DEV_H__

#include "type.h"

#define MAX_DEVICES 128

enum {
    DEV_UNKNOWN = 0,        /* 未知类型 */
    DEV_DISK,               /* 磁盘设备 */
};

struct _dev_op_t;
/* 设备类型 */
typedef struct _dev_type_t
{
    char name[32];          /* 设备名称 */
    int major;              /* 主设备号 */
    struct _dev_op_t *op;    /* 设备操作集合 */
}dev_type_t;

/* 设备 */
typedef struct _device_t
{
    struct _dev_type_t type;        /* 设备类型 */
    int mode;               /* 操作模式 */
    int minor;              /* 从设备号 */
    void *param;            /* 参数 */
    int open_count;         /* 打开计数 */
}device_t;

/* 设备操作接口 */
typedef struct _dev_op_t
{
    int (*open)(device_t *dev);
    int (*read)(device_t *dev, int begin, uchar *buf, int size);
    int (*write)(device_t *dev, int begin, const uchar *buf, int size);
    int (*ctrol)(device_t *dev, int cmd, char *arg[]);
    void (*close)(device_t *dev);
}dev_op_t;

int dev_open(int major, int minor, void *param);
int dev_read(int dev_id, int begin, uchar *buf, int size);
int dev_write(int dev_id, int begin, const uchar *buf, int size);
int dev_ctrol(int dev_id, int cmd, char *arg[]);
void dev_close(int dev_id);

#endif
