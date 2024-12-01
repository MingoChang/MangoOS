/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-25
 * ==================================
 */
#include "../include/page.h"
#include "../include/ipc.h"
#include "../include/string.h"
#include "../include/error.h"
#include "../include/task.h"

static mutex_t mutex;
extern task_t *current;

void page_init()
{
    bitmap_init();
    mutex_init(&mutex);
}

/* 分配多页物理内存 */
uint alloc_pages(int count)
{
    uint addr = 0;
    mutex_lock(&mutex);

    int index = bitmap_alloc_bits(count);
    if (index > 0) {
        addr = index * PAGE_SIZE;
    }

    mutex_unlock(&mutex);

    return addr;
}

/* 释放多页物理内存 */
void free_pages(uint addr, int count)
{
    mutex_lock(&mutex);
    bitmap_free_bits(addr, count);
    mutex_unlock(&mutex);
}

/* 分配多页虚拟内存页并映射物理内存 */
uint alloc_vpages(int dindex, int index, int count)
{
    page_dir_t *pde = (page_dir_t*)current->cr3;
    uint page = pde[dindex].value;
    if (!page) {
        page = alloc_pages(1);
        if (!page) {
            return -ENOMEM;
        }
        
        pde[dindex].value = page | PAGE_SHARED;
    }

    page = align_down(page, PAGE_SIZE);
    page_table_t *pte = (page_table_t*)page + index;

    do {
        uint pg = alloc_pages(1);
        if (!pg) {
            return -ENOMEM;
        }
        
        pte->value = pg | PAGE_SHARED;
        pte++;
    } while (count--);

    invalidate();
    return 0;
}

/* 释放多页虚拟内存页 */
void free_vpages(int dindex, int index, int count)
{
    page_dir_t *pde = (page_dir_t*)current->cr3;
    uint page = pde[dindex].value;
    if (!(page & PAGE_PRESENT)) {
        return;
    }

    page = align_down(page, PAGE_SIZE);
    page_table_t *pte = (page_table_t*)page + index;

    do {
        free_pages(align_down(pte->value, PAGE_SIZE), 1);
        pte->value = 0;
        pte++;
    } while(count--);

    /* 检查本页表对应的项目是否都没映射了 */
    pte = (page_table_t*)page;
    for (int i=0; i<1024; i++) {
        if (pte[0].value != 0) {
            return;
        }
    }

    pde->value = 0;
    free_pages(page, 1);
    invalidate();
}

/* 创建进程页目录 */
uint create_pde()
{
    page_dir_t *pde = (page_dir_t*)alloc_pages(1);
    if (pde == 0) {
        return 0;
    }

    kmemset((void*)pde, 0, PAGE_SIZE);
    
    /* 在loader.S中已经初始化过了内核的页目录 */
    page_dir_t *kernel_pde = (page_dir_t *)KERNEL_PAGE_DIR;

    /* loader中映射物理内存都映射到内核的地址空间中
     * 用户空间的映射到进程加载的时候再做映射 */
	uint user_start_index = pde_index(USER_MEMORY_BEGIN);
    for (int i=0; i<user_start_index; i++) {
        pde[i].value = kernel_pde[i].value;
    }

    return (uint)pde;
}

/* 释放进程页目录和页表 */
void destroy_pde(uint page_dir)
{
    uint user_start_index = pde_index(USER_MEMORY_BEGIN);
    page_dir_t *pde = (page_dir_t*)page_dir + user_start_index;

    for (int i=user_start_index; i<1024; i++) {
        if (!pde[i].present) {
            continue;
        }
        
        page_table_t *pte = (page_table_t*)(align_down(pde[i].value, PAGE_SIZE));
        for (int j=0; j<1024; j++) {
            if (!pte[j].present) {
                continue;
            }

            free_pages(align_down(pte[j].value, PAGE_SIZE), 1);
            pte[j].value = 0;
        }

        free_pages(align_down(pde[i].value, PAGE_SIZE), 1);
        pde[i].value = 0;
    }
}

