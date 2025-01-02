/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-3
 * ==================================
 */
#include "../include/disk.h"
#include "../include/dev.h"
#include "../include/arch.h"
#include "../include/error.h"
#include "../include/ipc.h"
#include "../include/string.h"
#include "../include/irq.h"

static mutex_t mutex;
static sem_t sem;
extern task_t *current;

/* 最多4个分区，第0个分区存储整个硬盘的信息 */
static part_info_t part_info[5];
extern void exception_handler_ide();
static int load_disk_info();
static int wait_interupt = 0;

/* 磁盘初始化 */
void disk_init()
{
   mutex_init(&mutex);
   sem_init(&sem, 0); 
   load_disk_info();
}

/* 发送磁盘指令，和loader中一样的操作，那边汇编实现，这边C语言实现 */
static void send_cmd(uint start_sec, int count, int cmd)
{
    outb(0x1F6, 0xE0);                  /* 0号磁盘 */

    outb(0x1F2, (uchar)(count >> 8));   /* 读取的扇区数高8位 */
    outb(0x1F3, (uchar)(start_sec >> 24));  /* 起始扇区的24-31位 */
    outb(0x1F4, 0x00);                  /* 起始扇区的32-39位 */
    outb(0x1F5, 0x00);                  /* 起始扇区的40-47位 */
    outb(0x1F2, (uchar)(count));        /* 读取扇区数的低8位 */
    outb(0x1F3, (uchar)(start_sec));    /* 起始扇区的低8位 */
    outb(0x1F4, (uchar)(start_sec >> 8));   /* 起始扇区的8-15位 */
    outb(0x1F5, (uchar)(start_sec >> 16));   /* 起始扇区的8-15位 */

    outb(0x1F7, (uchar)cmd);            /* 发送命令 */
}

/* 读磁盘数据 */
static void read_data(void *buf, uint size)
{
    ushort *data = (ushort*)buf;
    while (size > 0) {
        *data++ = inw(0x1F0);
        size -= 2;
    }
}

/* 写磁盘数据 */
static void write_data(const void* buf, uint size)
{
    ushort *data = (ushort*)buf;
    while (size > 0) {
        outw(0x1F0, *data++);
        size -= 2;
    }
}

/* 等待磁盘有有数据到达 */
static int wait_data()
{
    uchar status = 0;
    do {
        status = inb(0x1F7);
        /* 137 表示错误位｜数据可读 | 磁盘忙 */
        if ((status & 137) != 128) {
            break;
        }
    } while(1);

    /* 前面排除了忙位，这边排除错误位，剩下就是数据可读 */
    return (status & 1) ? -1 : 0;
}

/* 加载分区信息 */
static int load_part_info()
{
    ushort data[256] = {0};
    /* 发送读命令 */
    send_cmd(0, 1, 0x24);

    /* 此时磁盘终端没开启，需要等待有数据可读 */
    int err = wait_data();
    if (err) {
        return -ENODISK;
    }

    read_data(data, 256 * 2);

    /* 分区信息在引导扇区偏移446位置 */
    part_item_t *item = (part_item_t*)(data + 223);
    for (int i=0; i<4; i++, item++) {
        part_info_t *part = &part_info[i+1];
        part->type = item->fs_type;
        
        /* 无效分区 */
        if (item->fs_type == 0) {
            part->count = 0;
            part->begin_sec = 0;
            part->type = 0;
        } else {
            kmemcpy(part->name, "SDA0", 3);
            part->name[3] = 48 + i;
            part->begin_sec = item->relative_sec;
            part->count = item->count;
            part->type = item->fs_type;
        }
    }

    return 0;
}

/* 加载磁盘信息 */
static int load_disk_info()
{
    ushort data[256] = {0};
    /* 发送identify命令获取磁盘信息 */
    send_cmd(0, 0, 0xEC);
    int err = inb(0x1F7);
    if (!err) {
        return -ENODISK;
    }

    /* 此时磁盘终端没开启，需要等待有数据可读 */
    err = wait_data();
    if (err) {
        return -ENODISK;
    }

    read_data(data, 256 * 2);

    part_info_t *disk_info = &part_info[0];
    kstrcpy(disk_info->name, "SDA");
    disk_info->begin_sec = 0;
    disk_info->count = (uint)(data + 100);
    disk_info->type = 0;
    load_part_info();

    return 0;
}

/* 打开磁盘设备 */
int disk_open(device_t *dev)
{
    /* 分区号 */
    int part_index = dev->minor & 0xF;

    part_info_t *info = &part_info[part_index];
    if (info->count == 0) {
        return -ENOPART;
    }

    /* 设置磁盘终端处理函数 */
    irq_install(0x2E, exception_handler_ide);
    enable_8259(0x2E);

    return 0;
}

void do_handler_ide(exception_frame_t *frame)
{
    if (wait_interupt && current) {
        sem_notify(&sem);
    }
    pic_done(0x2E);
}

/* 关闭磁盘设备 */
void disk_close()
{
    disable_8259(0x2E);
}

/* 磁盘读 */
int disk_read(device_t *dev, int begin_sec, uchar *buf, int count)
{
    /* 分区号 */
    int part_index = dev->minor & 0xF;

    mutex_lock(&mutex);
    part_info_t *info = &part_info[part_index];
    if (info->count == 0) {
        return -ENOPART;
    }

    /* 期待磁盘中断 */
    wait_interupt = 1;
    send_cmd(info->begin_sec + begin_sec, count, 0x24);
    int i = 0;
    for (i=0; i<count; i++, buf+=512) {
        /* 这边需要加这个判断，因为磁盘初始化的需要用到读磁盘，这个时候还没有进程，还不能使用信号量 */
        if (current) {
            sem_wait(&sem);
        }

        int err = wait_data();
        if (err) {
            return -ENODISK;
        }

        read_data(buf, 512);
    }
    mutex_unlock(&mutex);
    
    return i;
}

/* 写磁盘 */
int disk_write(device_t *dev, int begin_sec, const uchar* buf, int count)
{
    /* 分区号 */
    int part_index = dev->minor & 0xF;

    mutex_lock(&mutex);
    part_info_t *info = &part_info[part_index];
    if (info->count == 0) {
        return -ENOPART;
    }

    /* 期待磁盘中断 */
    wait_interupt = 1;
    send_cmd(info->begin_sec + begin_sec, count, 0x34);
    int i = 0;
    for (i=0; i<count; i++, buf+=512) {
        write_data(buf, 512);
        /* 这边加这个判断理由同disk_read */
        if (current) {
            /* 等待写完成 */
            sem_wait(&sem);
        }

        int err = wait_data();
        if (err) {
            return -ENODISK;
        }
    }
    mutex_unlock(&mutex);
    
    return i;
}

/* 磁盘控制 */
int disk_ctrol(device_t *dev, int cmd, char *arg[])
{
    return 0;
}

dev_op_t op = {
    .open = disk_open,
    .read = disk_read,
    .write = disk_write,
    .ctrol = disk_ctrol,
    .close = disk_close
};

dev_type_t dev_disk = {
    .name = "disk",
    .major = DEV_DISK,
    .op = &op
};
