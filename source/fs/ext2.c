/*
 * ==================================
 * auther: Mingo
 * date: 2024-12-9
 * ==================================
 */
#include "../include/ext2.h"
#include "../include/dev.h"
#include "../include/memory.h"
#include "../include/error.h"
#include "../include/string.h"
#include "../include/ipc.h"
#include "../include/bitmap.h"
#include "../include/buffer.h"

extern uint timestamp;
extern list_t dirty_blocks;
static uchar *p_super_block = NULL;

int ext2_mount(fs_t *fs, int major, int minor)
{
    int dev_id = dev_open(major, minor, NULL);
    if (dev_id < 0) {
        return dev_id;
    }

    /* ext2超级块在所属分区偏移1024字节处(第3扇区开始），这边读取7个扇区：
     * 对于块大小是1K的ext2文件系统，块组描述符从第三块开始；对于块大小大
     * 于或者等于2K的ext2文件系统，块组描述符表从第二块开始。从1024字节偏
     * 移开始算就是第3扇区和第7扇区。所以这边读7个扇区都是够的 */
    uchar data[4096];
    int count = dev_read(dev_id, 2, data, 7);
    if (count <= 0) {
        return count;
    }

    /* 保存超级快内容，用户超级快内容磁盘写回 */
    p_super_block = kmalloc(4096);
    kmemcpy(p_super_block, data, 4096);
    
    /* 计算有多少个块组 */
    int group_count = *(uint*)(data + 4) / *(uint*)(data + 32);
    if (*(uint*)(data + 4) % *(uint*)(data + 32) != 0) {
        group_count++;
    }

    super_block_t *super_block = kmalloc(sizeof(super_block_t) + group_count * sizeof(group_desc_t));
    if (!super_block) {
        dev_close(dev_id);
        return -ENOMEM;
    }

    kstrcpy(super_block->name, "ext2");
    /* 以下内容均通过偏移量取到 */
    super_block->magic = data[57] * 256 + data[56];
    super_block->inode_count = *(uint*)(data + 0);
    super_block->block_count = *(uint*)(data + 4);
    super_block->used_block = *(uint*)(data + 8);
    super_block->free_block = *(uint*)(data + 12);
    super_block->free_inode = *(uint*)(data + 16);
    super_block->inode_size = data[89] * 256 + data[88];

    char log_size = *(uint*)(data + 24);
    super_block->block_size = 1024 << log_size;
    super_block->group_block = *(uint*)(data + 32);
    super_block->group_inode = *(uint*)(data + 40);
    super_block->group_count = group_count;

    /* 计算块组描述符位置 */
    /* 块大小等于1K的时候，块组描述符表在第3块，大于1K的时候，在第7块 */
    int pos = super_block->block_size > 1024 ? 6 : 2;
    for (int i=0; i<group_count; i++) {
        super_block->group_table[i].block_bitmap = *(uint*)(data + i * 32 + pos * 512 + 0);
        super_block->group_table[i].inode_bitmap = *(uint*)(data + i * 32 + pos * 512 + 4);
        super_block->group_table[i].inode_table = *(uint*)(data + i * 32 + pos * 512 + 8);
        super_block->group_table[i].free_block = *(ushort*)(data + i * 32 + pos * 512 + 12);
        super_block->group_table[i].free_inode = *(ushort*)(data + i * 32 + pos * 512 + 14);
        super_block->group_table[i].dir_inode = *(ushort*)(data + i * 32 + pos * 512 + 16);
        super_block->group_table[i].block_bitmap_size = (super_block->group_block + 7) / 8;
        super_block->group_table[i].inode_bitmap_size = (super_block->group_inode + 7) / 8;

        super_block->group_table[i].p_block_bitmap = kmalloc(super_block->group_table[i].block_bitmap_size);
        super_block->group_table[i].p_inode_bitmap = kmalloc(super_block->group_table[i].inode_bitmap_size);
    }

    /* 加载inode位图和数据块位图 */
    for (int i=0; i<group_count; i++) {
        int block_sec = super_block->group_table[i].block_bitmap * super_block->block_size / 512;
        int sec_count = (super_block->group_table[i].block_bitmap_size) / 512;
        /* 怕内核栈爆掉，data不能太大，所以8扇区读一次 */
        int offset = 0;
        do {
            if (sec_count > 8) {
                count = dev_read(dev_id, block_sec, data, 8);
            } else {
                count = dev_read(dev_id, block_sec, data, sec_count);
            }
            if (count <= 0) {
                while(i-- >= 0) {
                    kfree(super_block->group_table[i].p_block_bitmap);
                    kfree(super_block->group_table[i].p_inode_bitmap);
                }
                kfree(super_block);
                dev_close(dev_id);
                return -ENOMEM;
            }

            if (sec_count > 8) {
                kmemcpy(super_block->group_table[i].p_block_bitmap + offset * 512, data, 8 * 512);
            } else {
                kmemcpy(super_block->group_table[i].p_block_bitmap + offset * 512, data, sec_count * 512);
                if (super_block->group_table[i].block_bitmap_size % 512 != 0) {
                    kmemcpy(super_block->group_table[i].p_block_bitmap + (offset + sec_count) * 512, data + sec_count * 512, \
                            super_block->group_table[i].block_bitmap_size % 512);
                }
            }

            sec_count -= 8;
            offset++;
        } while (sec_count > 0);

        block_sec = super_block->group_table[i].inode_bitmap * super_block->block_size / 512;
        sec_count = (super_block->group_table[i].inode_bitmap_size) / 512;
        /* 怕内核栈爆掉，data不能太大，所以8扇区读一次 */
        offset = 0;
        do {
            if (sec_count > 8) {
                count = dev_read(dev_id, block_sec, data, 8);
            } else {
                count = dev_read(dev_id, block_sec, data, sec_count);
            }
            if (count <= 0) {
                while(i-- >= 0) {
                    kfree(super_block->group_table[i].p_block_bitmap);
                    kfree(super_block->group_table[i].p_inode_bitmap);
                }
                kfree(super_block);
                dev_close(dev_id);
                return -ENOMEM;
            }

            if (sec_count > 8) {
                kmemcpy(super_block->group_table[i].p_inode_bitmap + offset * 512, data, 8 * 512);
            } else {
                kmemcpy(super_block->group_table[i].p_inode_bitmap + offset * 512, data, sec_count * 512);
                if (super_block->group_table[i].inode_bitmap_size % 512 != 0) {
                    kmemcpy(super_block->group_table[i].p_inode_bitmap + (offset + sec_count) * 512, data + sec_count * 512, \
                            super_block->group_table[i].inode_bitmap_size % 512);
                }
            }

            sec_count -= 8;
            offset++;
        } while (sec_count > 0);

        /* 申请inode表数据缓存 */
        super_block->group_table[i].inode_begin = -1;
        super_block->group_table[i].inode_data = (inode_t*)kmalloc(PAGE_SIZE * 5);
    }


    fs->data = (void*)super_block;
    fs->dev_id = dev_id;

    super_block->fs = fs;

    mutex_t *mutex = kmalloc(sizeof(mutex_t));
    if (!mutex) {
        for (int i=0; i<group_count; i++) {
            kfree(super_block->group_table[i].p_block_bitmap);
            kfree(super_block->group_table[i].p_inode_bitmap);
            kfree(super_block->group_table[i].inode_data);
        }

        kfree(super_block);
        dev_close(dev_id);
        return -ENOMEM;
    }

    mutex_init(mutex);
    fs->mutex = mutex;

    return dev_id;
}

void ext2_umount(fs_t *fs)
{
    dev_close(fs->dev_id);
    super_block_t *super_block = (super_block_t*)fs->data;
    for (int i=0; i<super_block->group_count; i++) {
        if (super_block->group_table[i].p_block_bitmap != NULL) {
            kfree(super_block->group_table[i].p_block_bitmap);
        }

        if (super_block->group_table[i].p_inode_bitmap != NULL) {
            kfree(super_block->group_table[i].p_inode_bitmap);
        }

        kfree(super_block->group_table[i].inode_data);
    }

    kfree(p_super_block);

    kfree(fs->data);
    kfree(fs->mutex);
}

block_buffer_t *read_block(super_block_t *block, uint bno)
{
    block_buffer_t *bb = bget_block(block, bno);
    int count = 0;
    if (bb->b_state != 1) {
        if (bb->b_state == 2) {
            /* 这边先把内存写回磁盘，再使用这个内存块, 这边马上就要使用，需要同步写 */
            count = dev_write(block->fs->dev_id, bno *block->block_size / 512, bb->data, 8);
            if (count <= 0) {
                return NULL;
            }
        }

        count = dev_read(block->fs->dev_id, bno * block->block_size / 512, bb->data, 8);
        if (count <= 0) {
            return NULL;
        }

        bb->b_state = 1;
    }
    
    return bb;
}

static int write_inode(super_block_t *block, group_desc_t *group)
{
    uint inode_block = group->inode_begin;
    if (inode_block == -1) {
        return -1;
    }

    /* 一次多读数个inode表到缓存中 */
    inode_t *temp = group->inode_data;

    int num_in_cache = PAGE_SIZE * 5 / block->inode_size;
    /* 每块内存可存储几个inode */
    int num = block->block_size / block->inode_size;
    block_buffer_t* bb = NULL;
    int step = 0;

    /* 写回缓冲内容到磁盘 */
    for (int i=0; i<num_in_cache; i++) {
        if (i % num == 0) {
            bb = read_block(block, inode_block + i / num);
            if (!bb) {
                return -1;
            }
            step = 0;
        }

        *(uint*)(bb->data + step + 28) = temp->i_blocks;
        *(ushort*)(bb->data + step + 2) = temp->i_uid;
        *(ushort*)(bb->data + step + 24) = temp->i_uid;
        *(uint*)(bb->data + step + 4) = temp->i_size;
        *(uint*)(bb->data + step + 8) = temp->i_atime;;
        *(uint*)(bb->data + step + 16) = temp->i_mtime;;
        *(uint*)(bb->data + step + 12) = temp->i_ctime;;
        kmemcpy(bb->data + step + 40, temp->i_block, 15*4);

        /* 写块操作 */
        if (i % num == (num - 1) || i == (num_in_cache - 1 )) {
            dev_write(block->fs->dev_id, bb->block_no *block->block_size / 512, bb->data, 8);
        } 

        temp++;
        step += block->inode_size;
    }
}


/* 本函数中的缓存是按照super_block->inode_dize来计算的，而目前内存定义的inode_t并
 * 没有达到这个值，所以缓存还有大段没使用，这边并不是bug，而是后续inode_t的大小还
 * 会扩充，暂不处理这个问题 */
static inode_t *iget_inode(super_block_t *block, uint ino)
{
    inode_t *inode = NULL;

    if (ino < 1 || ino > block->inode_count) {
        return NULL;
    }

    /* 计算inode所属于快组和块组内的偏移 */
    uint group = (ino - 1) / block->group_inode;
    uint index = (ino - 1) % block->group_inode;

    group_desc_t *group_desc = &block->group_table[group];
    if (!group_desc) {
        return NULL;
    }

    /* 计算inode表的块号和偏移量 */
    uint inode_block = group_desc->inode_table + (index * block->inode_size) / block->block_size;
    uint offset = (index * block->inode_size) % block->block_size;

    /* 计算是否inode在缓存中*/
    if (group_desc->inode_begin != -1 && (inode_block - group_desc->inode_begin) < 5) {
        inode = group_desc->inode_data + (inode_block - group_desc->inode_begin) * block->block_size / \
                block->inode_size + offset / block->inode_size;
        return inode;
    }

    write_inode(block, group_desc);

    /* 一次多读数个inode表到缓存中 */
    inode_t *temp = group_desc->inode_data;

    int num_in_cache = PAGE_SIZE * 5 / block->inode_size;
    /* 每块内存可存储几个inode */
    int num = block->block_size / block->inode_size;
    block_buffer_t* bb = NULL;
    int step = 0;
    for (int i=0; i<num_in_cache; i++) {
        if (i % num == 0) {
            bb = read_block(block, inode_block + i / num);
            if (!bb) {
                return NULL;
            }
            step = 0;
        }
        
        temp->i_mode = bb->data[1+step] * 256 + bb->data[0+step];
        temp->i_blocks = *(uint*)(bb->data + step + 28);
        temp->i_uid = bb->data[3+step] * 256 + bb->data[2+step];
        temp->i_gid = bb->data[25+step] * 256 + bb->data[24+step];
        temp->i_size = *(uint*)(bb->data + step + 4);
        temp->i_atime = *(uint*)(bb->data + step + 8);
        temp->i_mtime = *(uint*)(bb->data + step + 16);
        temp->i_ctime = *(uint*)(bb->data + step + 12);
        kmemcpy(temp->i_block, bb->data + step + 40, 15*4);
        temp->sb = block;

        temp++;
        step += block->inode_size;
    }

    group_desc->inode_begin = inode_block;

    return group_desc->inode_data + offset / block->inode_size;
}

static void set_bit(uchar *data, uint index, int flag)
{
    if (flag == 1) {
        data[index / 8] |= 1 << (index % 8);
    } else {
        data[index / 8] |= 0 << (index % 8);
    }
}

static int ffs(uint word)
{
    if (word == 0) {
        return 0;
    }

    int bit = 1;
    while ((word & 1) == 0) {
        word >> 1;
        bit++;
    }

    return bit;
}

static int find_first_zero_bit(const uint *bitmap, uint size) {
    uint word, bit;
    uint i = 0;

    /* 每32位为一组检查是否有空位 */
    while(i < size / (sizeof(uint) * 8)) {
        word = bitmap[i];

        /*32个bit位不全1 */
        if (~word != 0) {
            /* 获取第一个为0的bit位索引 */
            bit = ffs(~word);

            /* 设置位已经背占用 */
            return i * sizeof(uint) * 8 + bit;
        }
        
        i++;
    }

    /* 返回size，表示没有找到 */
    return size;
}

static uint alloc_inode(super_block_t *block)
{
    int groups = block->inode_count / block->group_inode;
    int i = 0;

    for (i=0; i<groups; i++) {
        group_desc_t *group = &block->group_table[i];

        int ino = find_first_zero_bit((uint*)group->p_inode_bitmap, group->inode_bitmap_size);
        /* 本块组中没找到 */
        if (ino == group->inode_bitmap_size) {
            continue;
        }

        set_bit(group->p_inode_bitmap, ino, 1);
        block->free_inode--;
        group->free_inode--;

        return ino;
    }

    return 0;
} 

/* 在目录中查找inode */
static inode_t *find_inode_in_dir(file_t *file, inode_t *dir_inode, const char *name, int end) {
    if (!S_ISDIR(dir_inode->i_mode)) {
        return NULL;
    }
    
    int pos = 0;
    dir_t *dir_entry, *dir_empty = NULL;
    block_buffer_t *bb = NULL;
    inode_t *inode = NULL;
    while(pos < dir_inode->i_size) {
        /* 这边假设前12块磁盘空间(这边是4K * 12的空间)已经够存储目录下的各子目录和文件名 */
        bb = read_block(dir_inode->sb, dir_inode->i_block[pos / dir_inode->sb->block_size]);
        if (!bb) {
            return NULL;
        }

        dir_entry = (dir_t*)(bb->data);
        while((uchar*)dir_entry < bb->data + dir_inode->sb->block_size) {
            if (dir_entry->inode && !kmemcmp(dir_entry->name, name, kstrlen(name))) {
                inode = iget_inode(dir_inode->sb, dir_entry->inode);
                file->ino = dir_entry->inode;
                return inode;
            }

            if (!dir_entry->inode && dir_empty == NULL) {
                dir_empty = dir_entry;
            }

            dir_entry = (dir_t*)((uchar*)dir_entry + dir_entry->rec_len);
        }

        pos += dir_inode->sb->block_size;
    }

    /* 没有找到文件，通过文件的mode判断是否创建文件 */
    if ((file->mode & O_CREAT) == O_CREAT && end) {
        uint ino = alloc_inode(dir_inode->sb);
        inode = iget_inode(dir_inode->sb, ino);

        /* 内存的目录项增加目录 */
        dir_empty->inode = ino;
        kmemcpy(dir_empty->name, name, kstrlen(name));
        dir_empty->name_len = kstrlen(name);
        dir_empty->rec_len = 8 + kstrlen(name);

        /* 标记该块位脏块，需要写磁盘 */
        list_insert_tail(&dirty_blocks, &bb->dirty_node);

        return inode;
    }


    return NULL;
}

static const char *iget_name(const char *path, char* name, int *end)
{
    if (kstrlen(path) == 0) {
        return NULL;
    }

    while(*path == '/') {
        path++;
    }

    int i=0;
    while (*path && *path !='/' && i < MAX_NAME_LEN - 1) {
        name[i++] = *path++;
    }
    name[i] = '\0';

    while(*path == '/') {
        path++;
        *end = 0;
    }

    /* 返回更新后的路径 */
    return path;
}

static inode_t *find_inode_by_path(file_t *file, const char* path)
{
    super_block_t *block = (super_block_t*)file->fs->data;
    /* ext2文件系统，根目录的inode号固定为 2 */
    inode_t *inode = iget_inode(block, 2);
    inode_t *parent = NULL;
    
    char name[MAX_NAME_LEN];
    int end = 1;

    /* 解析路径 */
    while ((path = iget_name(path, name, &end)) != NULL) {
        parent = inode;
        inode = find_inode_in_dir(file, inode, name, end);
        if (!inode) {
            return NULL;
        }
        inode->parent = parent;
    }

    return inode;
}

uint iget_bno(inode_t *inode, uint index)
{
    /* 直接块 */
    if (index < 12) {
        return inode->i_block[index];
    }

    block_buffer_t *bb = NULL;
    /* 间接块 */
    if (index < 1036) {
        bb = read_block(inode->sb, inode->i_block[12]);
        if (!bb) {
            return -1;
        }

        return *(uint*)(bb->data + index - 12);
    }

    uint cur_index = 0;
    uint cur_bno = 0;

    /* 二级间接块 */
    if (index < 1049612) {
        bb = read_block(inode->sb, inode->i_block[13]);
        if (!bb) {
            return -1;
        }

        cur_index = (index - 1036) / 1024;
        cur_bno = *(uint*)(bb->data + cur_index);

        bb = read_block(inode->sb, cur_bno);
        if (!bb) {
            return -1;
        }

        return *(uint*)(bb->data + (index - 1036) % 1024);
    }

    /* 三级间接块 */
    bb = read_block(inode->sb, inode->i_block[14]);
    if (!bb) {
        return -1;
    }

    cur_index = (index - 1049612) / 1048576;
    cur_bno = *(uint*)(bb->data + cur_index);

    bb = read_block(inode->sb, cur_bno);
    if (!bb) {
        return -1;
    }

    cur_index = (index - 1049612) % 1048576 / 1024;
    cur_bno = *(uint*)(bb->data + cur_index);

    bb = read_block(inode->sb, cur_bno);
    if (!bb) {
        return -1;
    }

    cur_index = (index - 1049612) % 1048576 % 1024;
    return *(uint*)(bb->data + cur_index);
}

static int alloc_block(file_t *file)
{
    super_block_t *block = (super_block_t*)file->fs->data;
    inode_t *inode = file->inode;
    int gno = file->ino / block->group_inode;
    group_desc_t *group = &block->group_table[gno];

    int bno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
    int bindex = file->pos / block->block_size;

    /* 直接块 */
    if (bindex < 11) {
        inode->i_block[bindex + 1] = bno;
        set_bit(group->p_block_bitmap, bno, 1);
        return bno; 
    }

    block_buffer_t *bb = NULL; 

    /* 间接块 */
    if (bindex < 1035) {
        /* 当前要申请的块为间接块的第一块 */
        if (bindex == 11) {
            inode->i_block[12] = bno;
            set_bit(group->p_block_bitmap, bno, 1);

            bb = read_block(block, bno);
            if (!bb) {
                return -1;
            }

            int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
            *(uint*)bb->data = nbno;
            set_bit(group->p_block_bitmap, nbno, 1);

            return nbno;
        } else {
            bb = read_block(block, inode->i_block[12]);
            if (!bb) {
                return -1;
            }

            *(uint*)((uint*)bb->data + bindex - 12) = bno;
            set_bit(group->p_block_bitmap, bno, 1);
            return bno;
        }
    }

    /* 二级间接块 */
    if (bindex < 1049611) {
        /* 当前块为二级间接块的第一块 */
        if (bindex == 1035) {
            inode->i_block[13] = bno;
            set_bit(group->p_block_bitmap, bno, 1);
             
            bb = read_block(block, bno);
            if (!bb) {
                return -1;
            }

            int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
            *(uint*)bb->data = nbno;
            set_bit(group->p_block_bitmap, nbno, 1);
            
            bb = read_block(block, nbno);
            if (!bb) {
                return -1;
            }

            int nnbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
            *(uint*)bb->data = nnbno;
            set_bit(group->p_block_bitmap, nnbno, 1);
            return nnbno;
        /* 装满一个二级块 */
        } else if ((bindex - 1035) % 1024 == 0) {
            block_buffer_t *bb = read_block(block, inode->i_block[13]);
            if (!bb) {
                return -1;
            }

            *(uint*)((uint*)bb->data + (bindex - 1035) / 1024) = bno;
            set_bit(group->p_block_bitmap, bno, 1);

            bb = read_block(block, bno);
            if (!bb) {
                return -1;
            }

            int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
            *(uint*)bb->data = nbno;
            set_bit(group->p_block_bitmap, nbno, 1);
            return nbno;
        } else {
            bb = read_block(block, inode->i_block[13]);
            if (!bb) {
                return -1;
            }

            int anbo = *(uint*)((uint*)bb->data + (bindex - 1035) / 1024);
            bb = read_block(block, anbo);
            if (!bb) {
                return -1;
            }

            *(uint*)((uint*)bb->data + (bindex - 1035) % 1024) = bno;
            set_bit(group->p_block_bitmap, bno, 1);
            return bno;
        }
    }

    /* 三级间接块 */
    /* 当前要申请的块为三级第一个块 */
    if (bindex == 1049611) {
        inode->i_block[14] = bno;
        set_bit(group->p_block_bitmap, bno, 1);

        bb = read_block(block, bno);
        if (!bb) {
            return -1;
        }

        int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nbno;
        set_bit(group->p_block_bitmap, nbno, 1);
        
        bb = read_block(block, nbno);
        if (!bb) {
            return -1;
        }

        int nnbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nnbno;
        set_bit(group->p_block_bitmap, nnbno, 1);

        bb = read_block(block, nnbno);
        if (!bb) {
            return -1;
        }

        int nnnbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nnnbno;
        set_bit(group->p_block_bitmap, nnnbno, 1);

        return nnnbno;
    /* 装满一个二级块 */
    } else if ((bindex - 1049611) % 1048576 == 0) {
        bb = read_block(block, inode->i_block[14]);
        if (!bb) {
            return -1;
        }

        *(uint*)((uint*)bb->data + (bindex - 1049611) / 1048576) = bno;
        set_bit(group->p_block_bitmap, bno, 1);

        bb = read_block(block, bno);
        if (!bb) {
            return -1;
        }

        int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nbno;
        set_bit(group->p_block_bitmap, nbno, 1);

        bb = read_block(block, nbno);
        if (!bb) {
            return -1;
        }

        int nnbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nnbno;
        set_bit(group->p_block_bitmap, nnbno, 1);
        return nnbno;
    /* 装满一个三级块 */
    } else if ((bindex - 1049611) % 1048576 % 1024 == 0) {
        bb = read_block(block, inode->i_block[14]);
        if (!bb) {
            return -1;
        }

        int anbo = *(uint*)((uint*)bb->data + (bindex - 1049611) / 1048576);
        bb = read_block(block, anbo);
        if (!bb) {
            return -1;
        }
        
        *(uint*)((uint*)bb->data + (bindex - 1049611) % 1048576 / 1024) = bno;
        set_bit(group->p_block_bitmap, bno, 1);

        bb = read_block(block, bno);
        if (!bb) {
            return -1;
        }
        
        int nbno = find_first_zero_bit((uint*)group->p_block_bitmap, group->block_bitmap_size);
        *(uint*)bb->data = nbno;
        set_bit(group->p_block_bitmap, nbno, 1);

        return nbno;
    } else {
        bb = read_block(block, inode->i_block[14]);
        if (!bb) {
            return -1;
        }

        int anbo = *(uint*)((uint*)bb->data + (bindex - 1049611) / 1048576);
        bb = read_block(block, anbo);
        if (!bb) {
            return -1;
        }

        int annbo = *(uint*)((uint*)bb->data + (bindex - 1049611) % 1048576 / 1024);
        bb = read_block(block, annbo);
        if (!bb) {
            return -1;
        }
        
        *(uint*)((uint*)bb->data + (bindex - 1049611) % 1048576 % 1024) = bno;
        set_bit(group->p_block_bitmap, bno, 1);

        return bno;
    }

    return bno;
}

int ext2_open(file_t *file, const char *path)
{
    inode_t *inode = find_inode_by_path(file, path);
    if (!inode) {
        return -ENOTEXIST;
    }

    /* 是目录 */
    if (S_ISDIR(inode->i_mode)) {
        return -ENOTEXIST;
    }


    file->inode = inode;

    file->pos = 0;
    file->start_block = 0;
    file->cur_block = 0;

    return 0;
}

int ext2_read(file_t *file, char *data, int size)
{
    /* 一次最多读取 12 * 4096个字节 */
    if (size > 49152) {
        size = 49152;
    }

    /* 超过文件长度 */
    if (file->pos + size > file->inode->i_size) {
        size = file->inode->i_size - file->pos;
    }

    super_block_t *block = (super_block_t*)file->fs->data;
    /* 当前块起始位置 */
    int begin = file->pos % block->block_size; 
    int bno = iget_bno(file->inode, file->cur_block);
    int pos = 0;
    int r_size = size;
    block_buffer_t *bb = NULL;
    while (r_size > 0) {
        bb = read_block(block, bno);
        if (!bb) {
            return 0;
        }

        if (size <= block->block_size - begin) {
            kmemcpy(data + pos, bb->data + begin, r_size);
            file->pos += r_size;
            break;
        } else {
            kmemcpy(data + pos, bb->data + begin, block->block_size - begin);
            file->pos += block->block_size - begin;
        }

        pos += block->block_size - begin;
        r_size -= block->block_size - begin;
        begin = 0;
        file->cur_block++;
        bno = iget_bno(file->inode, file->cur_block);
    }

    return size;
}

int ext2_write(file_t *file, const char *data, int size)
{
    super_block_t *block = (super_block_t*)file->fs->data;
    inode_t *inode = file->inode;

    /* 当前块开始写入位置 */
    int begin = file->pos % block->block_size;
    int bno = iget_bno(inode, file->pos / block->block_size);
    int pos = 0;
    int w_size = size;
    while(w_size > 0) {
        block_buffer_t *bb = read_block(block, bno);
        if (!bb) {
            return 0;
        }

        /* 当前块空间够写入 */
        if (size <= block->block_size - begin) {
            kmemcpy(bb->data + begin, data + pos, w_size);
            file->pos += w_size;

            /* 记录为脏块，需要写入磁盘 */
            list_insert_tail(&dirty_blocks, &bb->dirty_node);
            break;
        /* 需要申请新块 */
        } else {
            kmemcpy(bb->data + begin, data + pos, block->block_size - begin);
            file->pos += block->block_size - begin;
            pos += block->block_size - begin;
            begin = 0;
            w_size -= block->block_size - begin;

            /* 记录为脏块，需要写入磁盘 */
            list_insert_tail(&dirty_blocks, &bb->dirty_node);
            break;
        }

        bno = alloc_block(file);
        if (bno < 0) {
            return 0;
        }
        block->used_block++;
        block->free_block--;
        inode->i_blocks++;

        int gno = file->ino / block->group_inode;
        group_desc_t *group = &block->group_table[gno];
        group->free_block--;

        file->cur_block++;
    }

    inode->i_size += size;
    inode->i_atime = timestamp;

    return size;
}

int ext2_seek(file_t *file, uint offset)
{
    if (offset >= 0) {
        file->pos = offset;
        super_block_t *block = (super_block_t*)file->fs->data;
        file->cur_block = file->pos / block->block_size;
    }

    return 0;
}

int ext2_sync(super_block_t *block)
{
    list_t *bn = NULL;
    block_buffer_t *bb = NULL;

    /* 写数据块到磁盘 */
    list_each(bn, &dirty_blocks) {
        bb = list_data(bn, block_buffer_t, dirty_node);    
        int count = dev_write(block->fs->dev_id, bb->block_no *block->block_size / 512, bb->data, 8);
        if (count <= 0) {
            return -1;
        }

        list_t *temp = bn->next;
        list_remove(bn);
        bn = temp;
    }

    /* 写inode和块位图到磁盘，一个块组的位图一般只占一个块，所以整个块一起更新，后续有时间再优化 */
    for (int i=0; i<block->group_count; i++) {
        int block_sec = block->group_table[i].block_bitmap * block->block_size / 512;
        int sec_count = block->group_table[i].block_bitmap_size / 512;
        int offset = 0;
        int count = 0;

        do {
            if (sec_count > 8) {
                count = dev_write(block->fs->dev_id, block_sec, block->group_table[i].p_block_bitmap + offset * 512, 8);
            } else {
                count = dev_write(block->fs->dev_id, block_sec, block->group_table[i].p_block_bitmap + offset * 512, sec_count);
            }
            if (count < 0) {
                return -EIO;
            }

            sec_count -= 8;
            offset++;
        } while (sec_count > 0);

        block_sec = block->group_table[i].inode_bitmap * block->block_size / 512;
        sec_count = block->group_table[i].inode_bitmap_size / 512;

        offset = 0;
        do {
            if (sec_count > 8) {
                count = dev_write(block->fs->dev_id, block_sec, block->group_table[i].p_inode_bitmap + offset * 512, 8);
            } else {
                count = dev_write(block->fs->dev_id, block_sec, block->group_table[i].p_inode_bitmap + offset * 512, sec_count);
            }
            if (count < 0) {
                return -EIO;
            }

            sec_count -= 8;
            offset++;
        } while (sec_count > 0);

        /* 写inode缓存到磁盘 */
        write_inode(block, &block->group_table[i]);
    }

    /* 写超级快和块组描述符内存到磁盘 */
    *(uint*)((uchar*)p_super_block + 8) = block->used_block;
    *(uint*)((uchar*)p_super_block + 12) = block->free_block;

    int pos = block->block_size > 1024 ? 6 : 2;
    for (int i=0; i<block->group_count; i++) {
        *(ushort*)((uchar*)p_super_block + i * 32 + pos * 512 + 12) = block->group_table[i].free_block;
        *(ushort*)((uchar*)p_super_block + i * 32 + pos * 512 + 14) = block->group_table[i].free_inode;
    }
    int count = dev_write(block->fs->dev_id, 2, p_super_block, 7);
    if (count < 0) {
        return -EIO;
    }


    return 0;
}

int ext2_close(file_t *file)
{
    /* 只读模式不需要写内存数据到磁盘 */
    if (file->mode & O_RDONLY == O_RDONLY) {
        return 0;
    }

    super_block_t *block = (super_block_t*)file->fs->data;
    ext2_sync(block);

    return 0;
}

static void ulink_inode(file_t *file, const char* name)
{
    int pos = 0;
    dir_t *dir_entry = NULL;
    block_buffer_t *bb = NULL;
    inode_t *inode = file->inode;

    while(pos < inode->i_size) {
        bb = read_block(inode->sb, inode->parent->i_block[pos / inode->sb->block_size]);
        if (!bb) {
            return;
        }

        dir_entry = (dir_t*)bb->data;
        while ((uchar*)dir_entry < bb->data + inode->i_size) {
            if (dir_entry->inode && !kmemcmp(dir_entry->name, name, kstrlen(name))) {
                dir_entry = 0;
            }

            dir_entry = (dir_t*)((uchar*)dir_entry + dir_entry->rec_len);
        }

        pos += inode->sb->block_size;
    }
}
        
static int get_name_index(const char* path)
{
    int start = 0;

    for (int i=0; i<kstrlen(path); i++) {
        if (*path++ == '/') {
            start = i + 1;
        }
    }

    return start;
}

int ext2_unlink(file_t *file, const char* path)
{
    inode_t *inode = find_inode_by_path(file, path);
    if (!inode) {
        return -ENOTEXIST;
    }

    group_desc_t *group = &inode->sb->group_table[file->ino / inode->sb->group_inode];

    block_buffer_t *bb = NULL;
    block_buffer_t *nbb = NULL;
    block_buffer_t *nnbb = NULL;
    /* 释放文件或者目录占的块 */
    for (int i=0; i<inode->i_blocks; i++) {
        if (i < 12) {
            /* 这边使用bget_block，表示数据块不在内存中的时候不需要读取，下同 */
            bb = bget_block(inode->sb, inode->i_block[i]);
            if (!bb) {
                return -EIO;
            }

            /* 在缓存中, 需要做处理 */
            if (bb->b_state) {
                bb->b_state = 0;
                set_bit(group->p_block_bitmap, bb->block_no, 0);
            }
        } else if (i < 1036) {
            if (i == 12) {
                bb = read_block(inode->sb, inode->i_block[12]);
                if (!bb) {
                    return -EIO;
                }
            }

            block_buffer_t *nbb = bget_block(inode->sb, *(uint*)(bb->data + (i - 12) * 4));
            if (!nbb) {
                return -EIO;
            }

            if (nbb->b_state) {
                nbb->b_state = 0;
                set_bit(group->p_block_bitmap, nbb->block_no, 0);
            }
           
            /* 释放间接块本身 */ 
            if (i == 1035 || i == inode->i_blocks - 1) {
                bb->b_state = 0;
                set_bit(group->p_block_bitmap, bb->block_no, 0);
            }
        } else if (i < 1049612) {
            if (i == 1036) {
                bb = read_block(inode->sb, inode->i_block[13]);
                if (!bb) {
                    return -EIO;
                }
            }

            /* 新一级间接块开始 */
            if ((i - 1036) % 1024 == 0) {
                nbb = read_block(inode->sb, *(uint*)(bb->data + (i-1036) / 1024 * 4));
                if (!nbb) {
                    return -EIO;
                }
            }

            nnbb = bget_block(inode->sb, *(uint*)(nbb->data + (i - 1036) % 1024 * 4));
            if (!nnbb) {
                return -EIO;
            }

            if (nnbb->b_state) {
                nnbb->b_state = 0;
                set_bit(group->p_block_bitmap, nnbb->block_no, 0);
            }

            /* 本一级间接块下面的最后一块，释放本块 */
            if ((i - 1036) % 1024 == 1023 || i == inode->i_blocks - 1) {
                if (nbb->b_state) {
                    nbb->b_state = 0;
                    set_bit(group->p_block_bitmap, nbb->block_no, 0);
                }
            }

            /* 释放二级间接块的头指针块 */
            if (i == 1049611 || i == inode->i_blocks - 1) {
                bb->b_state = 0;
                set_bit(group->p_block_bitmap, bb->block_no, 0);
            }
        } else {
            if (i == 1049612) {
                bb = read_block(inode->sb, inode->i_block[14]);
                if (!bb) {
                    return -EIO;
                }
            }

            /* 三级间接块的第一级开始 */
            if ((i - 1049612) % 1048576 == 0) {
                nbb = read_block(inode->sb, *(uint*)(bb->data + (i-1049612) / 1048576 * 4));
                if (!nbb) {
                    return -EIO;
                }
            }

            /* 三级间接块第二级开始 */
            if ((i - 1049612) % 1048576 % 1024 == 0) {
                nnbb = read_block(inode->sb, *(uint*)(nbb->data + (i-1049612) % 1048576 / 1024 * 4));
                if (!nnbb) {
                    return -EIO;
                }
            }

            block_buffer_t *nnnbb = bget_block(inode->sb, *(uint*)(nbb->data + (i - 1049612) % 1048576 % 1024 * 4));
            if (!nnnbb) {
                return -EIO;
            }

            if (nnnbb->b_state) {
                nnnbb->b_state = 0;
                set_bit(group->p_block_bitmap, nnnbb->block_no, 0);
            }

            /* 本级间接块下面的最后一块，释放本块 */
            if ((i - 1049612) % 1048576 % 1024 == 1023 || i == inode->i_blocks - 1) {
                if (nnbb->b_state) {
                    nnbb->b_state = 0;
                    set_bit(group->p_block_bitmap, nnbb->block_no, 0);
                }
            }

            /* 本级间接块下面的最后一块，释放本块 */
            if ((i - 1049612) % 1048576  == 1048575 || i == inode->i_blocks - 1) {
                if (nbb->b_state) {
                    nbb->b_state = 0;
                    set_bit(group->p_block_bitmap, nbb->block_no, 0);
                }
            }
            /* 释放二级间接块的头指针块 */
            if (i == inode->i_blocks - 1) {
                bb->b_state = 0;
                set_bit(group->p_block_bitmap, bb->block_no, 0);
            }
        }
    }
    
    /* 删除文件目录项 */
    ulink_inode(file, path + get_name_index(path));

    return 0;
}

fs_op_t ext2_op = {
    .mount = ext2_mount,
    .umount = ext2_umount,
    .open = ext2_open,
    .read = ext2_read,
    .write = ext2_write,
    .seek = ext2_seek,
    .sync = ext2_sync,
    .close = ext2_close,
    .unlink = ext2_unlink
};

