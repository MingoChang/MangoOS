/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-28
 * ==================================
 */
#ifndef __ERROR_H__
#define __ERROR_H__

#define EPERM   1   /* 操作不被允许 */
#define ENOENT  2   /* 没有这个文件或者目录 */
#define ESRCH   3   /* 没有这个进程 */
#define EINTR   4   /* 中断的系统调用 */
#define EIO     5   /* IO 错误 */
#define ENXIO   6   /* 没有这样的设备或者地址 */
#define EACCES  7   /* 没有权限 */
#define EFAULT  8   /* 无效的地址 */
#define ENOMEM  9   /* 内存耗尽 */

#endif
