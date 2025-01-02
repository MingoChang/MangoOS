/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-3
 * ==================================
 */
#include "../include/dev.h"
#include "../include/irq.h"
#include "../include/error.h"

extern dev_type_t dev_disk;

static device_t dev_table[MAX_DEVICES]; 

/* 支持的设备 */
static dev_type_t* support_table[] = {
    &dev_disk
};

/* 打开设备 */
int dev_open(int major, int minor, void *param)
{
    uint eflags = enter_critical();

    int free_index = -1;
    device_t *dev = NULL;
    for (int i=0; i<MAX_DEVICES; i++) {
       dev = &dev_table[i]; 
       if (dev->open_count) {
           /* 设备已经打开 */
           if (dev->type.major == major && dev->minor == minor) {
               dev->open_count++;
               leave_critical(eflags);
               return i;
           }
       } else {
           if (free_index == -1) {
               free_index = i;
           }
       }
    }

    dev_type_t *dev_type = NULL;
    /* 找到一个空项 */
    if (free_index != -1) {
        dev = &dev_table[free_index];
        for (int i=0; i< sizeof(support_table)/sizeof(support_table[0]); i++) {
            dev_type = support_table[i];
            if (major == dev_type->major) {
                break;
            }
        }

        if (dev_type) {
            dev->type = *dev_type;
            dev->minor = minor;
            dev->param = param;

            int err = dev->type.op->open(dev);
            if (!err) {
                dev->open_count = 1;
                leave_critical(eflags);
                return free_index;
            }
        }
    }

    leave_critical(eflags);
    return -ENODEV;
}

/* 关闭设备 */
void dev_close(int dev_id)
{
    device_t *dev = &dev_table[dev_id];

    uint eflags = enter_critical();

    if (--dev->open_count == 0) {
        dev->type.op->close(dev);
    }
    leave_critical(eflags);
}

/* 读数据 */
int dev_read(int dev_id, int begin, uchar *buf, int size)
{
    device_t *dev = &dev_table[dev_id];

    return dev->type.op->read(dev, begin, buf, size);
}

/* 写数据 */
int dev_write(int dev_id, int begin, const uchar *buf, int size)
{
    device_t *dev = &dev_table[dev_id];
    
    return dev->type.op->write(dev, begin, buf, size);
}

/* 设备控制 */
int dev_ctrol(int dev_id, int cmd, char *arg[])
{
    device_t *dev = &dev_table[dev_id];

    return dev->type.op->ctrol(dev, cmd, arg);
}
