/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-23
 * ==================================
 */
#include "../include/bitmap.h"
#include "../include/string.h"

/* 内存位图起始位置放置在0x10400处 */
static bit_map_t bitmap = {0, (uchar*)BITMAP_START};

static void bitmap_set_bytes(int sbytes , int nbytes, int used)
{
    if (used) {
        kmemset(bitmap.base + sbytes, 0xff, nbytes);
    } else {
        kmemset(bitmap.base + sbytes, 0x00, nbytes);
    }
}

/* 填充一个字节的连续count个比特位, up位1，往高地址填充，up位0，往低地址填充 */
static void bitmap_set_bits(bit_map_t *bitmap, int index, int bit, int count, int up)
{
    int move = 0;

    /* 往低位填充时，下标从0开始，要填充到0位，所以要减1 */
    if (!up) {
        move = -1;
    }

    do {
        if (bit) {
            bitmap->base[(index + move) / 8] |= 1 << ((index + move) % 8);
        } else {
            bitmap->base[(index + move) / 8] &= ~(1 << ((index + move) % 8));
        }

        if (up) {
            move++;
        } else {
            move--;
        }

    } while(--count > 0);
}

/* 获取比特位对应的状态 */
static int bitmap_get_bit(bit_map_t *bitmap, int index)
{
    return (bitmap->base[index / 8] & (1 << (index % 8))) ? 1 : 0;
}


void bitmap_init()
{
    uint nsize = 0;
    uint start = 0;
    uint end = 0;
    uint nbytes = 0;

    /* 先设置最大4G内存对应的位图为已使用 */
    bitmap_set_bytes(0, 128 * 1024, 1);

    /* 在loader中，我们将内存布局读取到了0x10000的位置 */
    mem_layout_t *layout = (mem_layout_t*)0x10000;
    do {
        nsize += layout->size;

        /* 保留内存标记为已使用 */
        if (layout->type != 1) {
            layout++;
            continue;
        }

        /* 内核 + 4M页表 + 1K页目录 标记为已经使用 */
        if (layout->start + layout->size <= FREE_MEM_START) {
            layout++;
            continue;
        }

        /* 起始地址小于内核目前空前地址的话，设置起始地址为目前空闲地址
         * 起始地址向上4K对齐 */
        if (layout->start < FREE_MEM_START) {
            start = (FREE_MEM_START + (PAGE_SIZE - 1)) / PAGE_SIZE;
        } else {
            start = (layout->start + (PAGE_SIZE -1 )) / PAGE_SIZE;
        }

        /* 一个字节表示8页，不足一个字节的话，后续的比特位表示的的页设置为空闲 */
        if (start % 8 != 0) {
            bitmap_set_bits(&bitmap, start, 0, 8 - start % 8, 1);
            start = (start + 7) / 8;
        } else {
            start = start / 8;
        }

        /* 本快内存的开始地址加上结束地址，就是本块内存的最后一个可用地址
         * 最后一个可用地址向下4K对齐 */
        end = (layout->start + layout->size) / PAGE_SIZE;
        if (end % 8 !=0) {
            bitmap_set_bits(&bitmap, end, 0, end % 8, 0);
        }
        end = end / 8;

        nbytes = end - start; 
        bitmap_set_bytes(start, nbytes, 0);

        layout++;

    } while(layout->start != 0);

    bitmap.nbits = nsize % PAGE_SIZE == 0 ? nsize / PAGE_SIZE : (nsize + PAGE_SIZE - 1) / PAGE_SIZE;
}

int bitmap_alloc_bits(int count)
{
    int cur_index = 0;
    int begin_index = 0;
    
    while (cur_index < bitmap.nbits) {
        if (bitmap_get_bit(&bitmap, cur_index++) != 0) {
            begin_index = cur_index;
            continue;
        } 

        /* 找到连续的n个bit位 */
        if (cur_index - begin_index >= count) {
            bitmap_set_bits(&bitmap, begin_index, 1, count, 1); 
            return begin_index;
        }
    }

    return -1;
}

void bitmap_free_bits(uint addr, int count)
{
    int index = addr / PAGE_SIZE;
    bitmap_set_bits(&bitmap, index, 0, count, 1);
}

