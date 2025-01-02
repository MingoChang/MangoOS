/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-6
 * ==================================
 */
#include "../include/fs.h"
#include "../include/disk.h"
#include "../include/log.h"
#include "../include/string.h"
#include "../include/file.h"
#include "../include/task.h"
#include "../include/error.h"
#include "../include/buffer.h"

static fs_t *root = NULL;
static fs_t fs_table[MAX_MOUNT];   /* 最多支持挂在10个文件系统 */

extern task_t *current;
extern fs_op_t ext2_op;

static fs_op_t* op_from_fs(int type)
{
    switch(type) {
        case FS_EXT2:
            return &ext2_op;
        default:
            return NULL;
    }
}

/* 挂在文件系统 */
static fs_t* mount(int type, char* mount_point, int major, int minor)
{
    fs_t * fs = NULL;

    int free_index = -1;
    int i=0;
    for (i; i<MAX_MOUNT; i++) {
        fs = &fs_table[i];
        /* 目录已经挂在了 */
        if (!kmemcmp(fs->mount_point, mount_point, kstrlen(mount_point))) {
            return NULL;
        }

        if (fs->type == 0 && free_index == -1) {
            free_index = i;
        } 
    }
    
    /* 挂满了 */
    if (free_index == -1) {
        return NULL;
    }

    fs = &fs_table[free_index];
    kstrcpy(fs->mount_point, mount_point);

    fs_op_t *op = op_from_fs(type);
    fs->type = type;
    fs->op = op;

    /* 执行挂载动作 */
    if (op->mount(fs, major, minor) < 0) {
        return NULL;
    }

    return fs;
}

void fs_init()
{
    disk_init();
    file_table_init();
    bb_init();

    root = mount(FS_EXT2, "/", 1, 1);
    if (!root) {
        /* 根文件系统挂在失败，直接down机 */
        kprintf("Init file system fail!");
        while (1) {
            __asm__ __volatile__("hlt");
        }
    }

}

int sys_open(const char* name, int mode)
{
    file_t *file = alloc_file();
    if (!file) {
        return -1;
    }

    int fd = task_alloc_fd(file);
    if (fd < 0) {
        return -ETMFILES;
    }

    /* 查找打开文件属于挂在的系统 */
    fs_t *fs = NULL;
    for (int i=0; i<MAX_MOUNT; i++) {
        fs_t *temp_fs = &fs_table[i];
        if (kmemcmp(temp_fs->mount_point, name, kstrlen(temp_fs->mount_point)) == 0) {
            fs = temp_fs;
            break;
        }
    }

    if (!fs) {
        fs = root;
    }

    file->mode = mode;
    file->fs = fs;

    mutex_lock(fs->mutex);
    int err = fs->op->open(file, name);
    if (err < 0) {
        mutex_unlock(fs->mutex);
        task_free_fd(fd);
        free_file(file);
        return err;
    }
    mutex_unlock(fs->mutex);

    /* 追加模式 */
    if ((mode & O_APPEND) == O_APPEND) {
        file->pos = file->inode->i_size;
    }

    return fd;
}

int sys_read(int fd, char* data, int len)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }
    task_t *task = current;

    file_t *file = task->open_file_table[fd];
    if (file == NULL) {
        return -1;
    }

    fs_t *fs = file->fs;
    mutex_lock(fs->mutex);
    int err = fs->op->read(file, data, len);
    if (err < 0) {
        mutex_unlock(fs->mutex);
        return err;
    }
    mutex_unlock(fs->mutex);

    return err;
}

int sys_write(int fd, char* data, int len)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }
    task_t *task = current;

    file_t *file = task->open_file_table[fd];
    if (file == NULL) {
        return -1;
    }

    fs_t *fs = file->fs;
    mutex_lock(fs->mutex);
    int err = fs->op->write(file, data, len);
    if (err < 0) {
        mutex_unlock(fs->mutex);
        return err;
    }
    mutex_unlock(fs->mutex);

    return err;
}

int sys_seek(int fd, int offset)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }
    task_t *task = current;

    file_t *file = task->open_file_table[fd];
    if (file == NULL) {
        return -1;
    }

    fs_t *fs = file->fs;
    mutex_lock(fs->mutex);
    fs->op->seek(file, offset);
    mutex_unlock(fs->mutex);

    return 0;
}

int sys_sync()
{
    for (int i=0; i<MAX_MOUNT; i++) {
        fs_t *fs = &fs_table[i];
        super_block_t *block = (super_block_t*)fs->data;

        mutex_lock(fs->mutex);
        fs->op->sync(block);
        mutex_unlock(fs->mutex);
    }
    
    return 0;
}

int sys_close(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }
    task_t *task = current;

    file_t *file = task->open_file_table[fd];
    if (file == NULL) {
        return -1;
    }

    free_file(file);
    if (file->ref == 0) {
        fs_t *fs = file->fs;
        mutex_lock(fs->mutex);
        fs->op->close(file);
        mutex_unlock(fs->mutex);
    }
    task_free_fd(fd);

    return 0;
}

int sys_unlink(const char *name)
{
    file_t *file = alloc_file();
    if (!file) {
        return -1;
    }

    int fd = task_alloc_fd(file);
    if (fd < 0) {
        return -ETMFILES;
    }

    fs_t *fs = NULL;
    for (int i=0; i<MAX_MOUNT; i++) {
        fs_t *temp_fs = &fs_table[i];
        if (kmemcmp(temp_fs->mount_point, name, kstrlen(temp_fs->mount_point)) == 0) {
            fs = temp_fs;
            break;
        }
    }

    if (!fs) {
        fs = root;
    }

    file->fs = fs;

    mutex_lock(fs->mutex);
    int err = fs->op->unlink(file, name);
    if (err < 0) {
        mutex_unlock(fs->mutex);
        free_file(file);
        task_free_fd(fd);

        return err;
    }
    mutex_unlock(fs->mutex);
    free_file(file);
    task_free_fd(fd);
   
    return err;
}

