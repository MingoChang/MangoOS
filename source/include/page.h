/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-25
 * ==================================
 */
#ifndef __PAGE_H__
#define __PAGE_H__

#include "type.h"
#include "bitmap.h"

#define align_down(addr, align) \
    ((addr) & ~((align) - 1))

#define align_up(addr, align) \
    (((addr) + ((align) + 1 )) & ~((align) - 1))

#define invalidate() \
    __asm__ __volatile__("movl %%cr3, %%eax\n\tmovl %%eax, %%cr3" : : : "ax")

#define USER_MEMORY_BEGIN 0x40000000
#define USER_STACK_SIZE (500 * 4096)

#define PAGE_PRESENT    1 << 0
#define PAGE_RW         1 << 1
#define PAGE_USER       1 << 2
#define PAGE_ACCESSED   1 << 5
#define PAGE_DIRTY      1 << 6
#define PAGE_COW        1 << 9

#define PAGE_SHARED     (PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_ACCESSED | PAGE_COW)

#pragma pack(1)

/* 也目录结构
|<------ 31~12------>|<------ 11~0 --------->| 比特
                     |b a 9 8 7 6 5 4 3 2 1 0| 
|--------------------|-|-|-|-|-|-|-|-|-|-|-|-| 占位
|<-------index------>| AVL |G|P|0|A|P|P|U|R|P| 属性
                             |S|   |C|W|/|/|
                                   |D|T|S|W|
*/
typedef union _page_dir_t
{
    uint value;
    struct {
        uint present:1;     /* 也是否存在内存中 */
        uint write_able:1;  /* 是否可写 */
        uint privilege:1;   /* 页目录特权级 */
        uint write_through:1; /* 页目录缓存策略 */
        uint cached:1;      /* 是否能缓存 */
        uint accessed:1;    /* 是否已经访问 */
        uint dirty:1;       /* 脏位,页目录中设置为0 */
        uint ps:1;			/* PS位，设置为0，表示4KB页 */
		uint global:1;		/* 页目录中没使用 */
		uint avl:3;			/* 页目录中没使用 */
		uint base:20;		/* page table物理地址 */
	};
}page_dir_t;


/* 页表结构
|<------ 31~12------>|<------ 11~0 --------->| 比特
                     |b a 9 8 7 6 5 4 3 2 1 0|
|--------------------|-|-|-|-|-|-|-|-|-|-|-|-| 占位
|<-------index------>| AVL |G|P|D|A|P|P|U|R|P| 属性
                             |A|   |C|W|/|/|
                             |T|   |D|T|S|W|
*/
typedef union _page_table_t
{
    uint value;
    struct {
        uint present:1;     /* 也是否存在内存中 */
        uint write_able:1;  /* 是否可写 */
        uint privilege:1;   /* 页目录特权级 */
        uint write_through:1; /* 页目录缓存策略 */
        uint cached:1;      /* 是否能缓存 */
        uint accessed:1;    /* 是否已经访问 */
        uint dirty:1;       /* 脏位,页目录中设置为0 */
        uint pat:1;			/* 和cache一起定义缓存方式 */
		uint global:1;		/* 页目录中没使用 */
		uint avl:3;			/* 页目录中没使用 */
		uint base:20;		/* page table物理地址 */
	};
}page_table_t;

#pragma pack()

static inline uint read_cr3()
{
    uint value;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(value));

    return value;
}

static inline void write_cr3(uint value)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(value) : "memory");
}

/* 通过线性地址获取在PDE表中的索引 */
static inline uint pde_index(uint addr)
{
    int index = addr >> 22;
    return index;
}

/* 通过线性地址获取在PTE表中的索引 */
static inline uint pte_index(uint addr)
{
    return (addr >> 12) & 0x3FF;
}

void page_init();
uint alloc_pages(int count);
void free_pages(uint addr, int count);
uint create_pde();
void destroy_pde(uint page_dir);
uint alloc_vpages(page_dir_t* pde, uint addr, uint size);
void free_vpages(page_dir_t* pde, uint addr, uint size);
int copy_from_user(void* to, const void* from, uint size);
int copy_to_user(void* to, const void* from, uint size);

#endif
