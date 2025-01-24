/*
 * ==================================
 * auther: Mingo
 * date: 2024-11-15
 * ==================================
 */
#include "../include/task.h"
#include "../include/string.h"
#include "../include/sched.h"
#include "../include/queue.h"
#include "../include/irq.h"
#include "../include/error.h"
#include "../include/page.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/file.h"
#include "../include/fs.h"
#include "../include/elf.h"
#include "../include/log.h"

static int pid = 0;
extern queue_t task_queue;
extern queue_t ready_task_queue;
extern queue_t sleep_task_queue;
extern task_t *current;
task_t *idle_task = NULL;

/* 分配任务结构 */
task_t* alloc_task()
{
    /* 结构体和内核栈共4K */
    task_t *task = (task_t*)kmalloc(2 * PAGE_SIZE);
    return task;
}

/* 释放任务结构 */
static void free_task(task_t *task)
{
    kfree((void*)task);
}

int task_alloc_fd(file_t *file)
{
    task_t *task = current;
    for (int i=0; i<TASK_OPEN_MAX_FILES; i++) {
        if (task->open_file_table[i] == NULL) {
            task->open_file_table[i] = file;
            return i;
        }
    }

    return -1;
}

void task_free_fd(int fd)
{
    task_t *task = current;
    if (fd >= 0 && fd < TASK_OPEN_MAX_FILES) {
        task->open_file_table[fd] = NULL;
    }
}

int task_init(task_t *task, const char *name, uint entry)
{
    task->pid = pid++; 
    kstrcpy(task->name, name);   
    /* 时间片初始化为1 */
    task->slice = TASK_SLICE;

    uint *pesp = (uint*)((void*)task + 8192);
    if (pesp) {
        *(--pesp) = entry; /* __switch_to中的ret会返回到这个地址 */
        *(--pesp) = 0;     /* 进程切换的时候__switch_to会自动插入%%ebx，所以还需一个空间来存放 */
        *(--pesp) = 0;
        *(--pesp) = (1 << 1) | (1 << 9);
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = (uint)task;
    }
    task->esp = (uint)pesp;
    task->cr3 = create_pde();
    task->state = TASK_CREATED;

    task->tss.esp0 = (uint)pesp;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.cr3 = task->cr3;

    /* 空闲进程在调度的地方需要使用 */
    if (entry == 0) {
        idle_task = task;
    }

    uint eflags = enter_critical();
    queue_insert_tail(&task_queue, &task->q);
    queue_insert_tail(&ready_task_queue, &task->rq);
    leave_critical(eflags);

    return 0;
}

/* 进程睡眠 */
static void task_sleep(task_t *task, uint ticks)
{
    if (ticks == 0) {
        return;
    }

    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;

    uint eflags = enter_critical();
    queue_remove(&task->rq);
    queue_insert_tail(&sleep_task_queue, &task->rq);
    leave_critical(eflags);
}

/* 进程唤醒 */
static void task_wakeup(task_t *task)
{
    uint eflags = enter_critical();
    queue_remove(&task->rq);
    queue_insert_tail(&ready_task_queue, &task->rq);
    task->state = TASK_RUNNING;
    leave_critical(eflags);
}

/* 定时器内检测进程时间片是否使用完 */
void task_tick()
{
    uint eflags = enter_critical();
    if (--current->slice == 0) {
        current->slice = TASK_SLICE;
        /* 将进程移动到运行队列的尾部 */
        queue_remove(&current->rq);
        queue_insert_tail(&ready_task_queue, &current->rq);
    }

    /* 查看睡眠队列，是否有进程该唤醒了 */
    queue_t *q = queue_first(&sleep_task_queue);
    while(q != &sleep_task_queue) {
        /* 本节点的后续节点要先取，因为wakeup会吧节点从睡眠队列中移到就绪队列，则next是就绪队列中的nexit */
        queue_t *next = q->next;
        task_t *task = queue_data(q, task_t, rq);
        if (--task->sleep_ticks == 0) {
            task_wakeup(task);
        }

        q = next;
    }

    schedule();
    leave_critical(eflags);
}

/* 进程主动放弃CPU */
void task_yield()
{
    uint eflags = enter_critical();
    /* 将进程移动到运行队列的尾部 */
    queue_remove(&current->rq);
    queue_insert_tail(&ready_task_queue, &current->rq);

    schedule();
    leave_critical(eflags);
}

/* 睡眠 */
void sys_sleep(uint ms)
{
    uint eflags = enter_critical();
    task_sleep(current, ms / TASK_SLICE);
    schedule();
    leave_critical(eflags);
}

static uint trans_vtop(page_dir_t* pde, uint vaddr)
{
    int dindex = pde_index(vaddr);
    int index = pte_index(vaddr);

    uint page = pde[dindex].value;
    if (!page) {
        return 0;
    }
    
    page = align_down(page, PAGE_SIZE);
    page_table_t *pg = (page_table_t*)page + index;

    return align_down(pg->value, PAGE_SIZE) + (vaddr & (PAGE_SIZE - 1));
}

static int load_phead(int fd, elf_phead_t *p_head, uint page_dir)
{
    /* 分配内存 */
    int err = alloc_vpages((page_dir_t*)page_dir, p_head->p_vaddr, p_head->p_memsz);
    if (err < 0) {
        return err;
    }

    if (sys_seek(fd, p_head->p_offset) < 0) {
        return -1;
    }

    uint vaddr = p_head->p_vaddr;
    int size = p_head->p_filesz;
    while (size > 0) {
        /* 取得物理地址 */
        uint paddr = trans_vtop((page_dir_t*)page_dir, vaddr);
        if (paddr == 0) {
            free_vpages((page_dir_t*)page_dir, p_head->p_vaddr, p_head->p_memsz);
            return -1;
        }

        if (sys_read(fd, (char*)paddr, (size > PAGE_SIZE) ? PAGE_SIZE : size) <= 0) {
            free_vpages((page_dir_t*)page_dir, p_head->p_vaddr, p_head->p_memsz);
            return -1;
        }

        size -= PAGE_SIZE;
        vaddr += PAGE_SIZE;
    }

    return 0;
}

static int load_elf_file(const char* name, uint page_dir)
{
    elf_head_t head;
    elf_phead_t p_head;

    int fd = sys_open(name, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    /* 读取文件头 */
    int count = sys_read(fd, (char*)&head, sizeof(elf_head_t));
    if (count <= 0) {
       sys_close(fd);
       return -1;
    } 

    /* 检查文件是否为ELF文件 */
    if ((head.e_ident[0] != 0x7F) || kmemcmp(&head.e_ident[1], "ELF", 3)) {
       sys_close(fd);
       return -1;
    }

    /* 可执行文件，并且匹配x86处理器 */
    if ((head.e_type != 2) || (head.e_machine != 3)) {
       sys_close(fd);
       return -1;
    }

    /* 程序头必须存在 */
    if ((head.e_phentsize == 0) || (head.e_phoff == 0)) {
       sys_close(fd);
       return -1;
    }

    /* 加载程序 */
    uint offset = head.e_phoff;
    for (int i=0; i<head.e_phnum; i++) {
        if (sys_seek(fd, offset) < 0) {
            sys_close(fd);
            return -1;
        }


        count = sys_read(fd, (char*)&p_head, sizeof(elf_phead_t));
        if (count <= 0) {
           sys_close(fd);
           return -1;
        } 

        /* 判断是否可加载的类型，地址为用户空间 */
        if (p_head.p_type != 1 || p_head.p_vaddr < USER_MEMORY_BEGIN) {
            continue;
        } 

        /* 加载各程序头到内存中 */
        int err = load_phead(fd, &p_head, page_dir);
        if (err < 0) {
            sys_close(fd);
            return -1;
        }

        offset += head.e_phentsize;
    }

    sys_close(fd);
    return head.e_entry;
}

static int copy_from_user_n(page_dir_t* pde, void* to, const void* from, uint size)
{
    /* 1-4G空间属于用户空间 */
    if ((int)from < 0x40000000 || (int)(from + size) > 0xFFFFFFFF) {
        return -1;
    }

    while (size > 0) {
        uint to_copy = size < PAGE_SIZE - ((int)from & (PAGE_SIZE - 1)) ? size : PAGE_SIZE - ((int)from & (PAGE_SIZE - 1));

        uint paddr = trans_vtop(pde, (uint)from);

        kmemcpy(to, (const void*)paddr, to_copy);

        size -= to_copy;
        from += to_copy;
        to += to_copy;
    }

    return 0;
}

int copy_from_user(void* to, const void* from, uint size)
{
    page_dir_t * pde = (page_dir_t*)current->cr3;
    return copy_from_user_n(pde, to, from, size);
}

static int copy_to_user_n(page_dir_t* pde, void* to, const void* from, uint size)
{
    /* 2-4G空间属于用户空间 */
    if ((uint)to < 0x40000000 || (uint)(to + size) > 0xFFFFFFFF) {
        return -1;
    }

    while (size > 0) {
        uint to_copy = size < PAGE_SIZE - ((int)to & (PAGE_SIZE - 1)) ? size : PAGE_SIZE - ((int)to & (PAGE_SIZE - 1));

        uint paddr = trans_vtop(pde, (int)to);

        kmemcpy((void*)paddr, from, to_copy);

        size -= to_copy;
        from += to_copy;
        to += to_copy;
    }

    return 0;
}

int copy_to_user(void* to, const void* from, uint size)
{
    page_dir_t *pde = (page_dir_t*)current->cr3;
    return copy_to_user_n(pde, to, from, size);
}

static uint copy_args_envs(page_dir_t *new_pde, char **argv, char **env)
{
    char **user_argv;
    char **user_env;

    char *stack_top = (char*)0xFFFFFFFF;
    
    char *table_arg[512];
    char *table_env[512];

    int argc = 0;
    if (!argv) {
        /* 填充参数个数 */
        stack_top -= sizeof(uint);
        copy_to_user_n(new_pde, stack_top, &argc, sizeof(uint));

        return (uint)stack_top;
    }

    for (argc = 0; argv[argc]; argc++) {
        stack_top -= kstrlen(argv[argc]) + 1;
        /* 多拷贝一个结尾符 */
        copy_to_user_n(new_pde, stack_top, argv[argc], kstrlen(argv[argc]) + 1); 
        table_arg[argc] = stack_top;
    }
    table_arg[argc] = NULL;

    int envc = 0;
    for (envc=0; env[envc]; envc++) {
        stack_top -= kstrlen(env[envc]) + 1;
        /* 多拷贝一个结尾符 */
        copy_to_user_n(new_pde, stack_top, env[envc], kstrlen(env[envc]) + 1); 
        table_env[envc] = stack_top;
    }
    table_env[envc] = NULL;

    /* 填充env指针表 */
    for (int i=0; i<=envc; envc++) {
        stack_top -= sizeof(char*);
        copy_to_user_n(new_pde, stack_top, table_env[envc], sizeof(char*));
    }
     
    /* 填充argv指针表 */
    for (int i=0; i<=argc; argc++) {
        stack_top -= sizeof(char*);
        copy_to_user_n(new_pde, stack_top, table_arg[argc], sizeof(char*));
    }

    /* 填充参数个数 */
    stack_top -= sizeof(uint);
    copy_to_user_n(new_pde, stack_top, &argc, sizeof(uint));

    return (uint)stack_top;
}

void switch_to_user_mode(uint entry, uint stack_top)
{
    uint user_cs = USER_SELECTOR_CS;
    uint user_ds = USER_SELECTOR_DS;
    uint eflags = 0x202;

    __asm__ __volatile__(
            /* 设置用户态的段寄存器 */
            "mov %0, %%ds\n\t"
            "mov %0, %%es\n\t"
            "mov %0, %%fs\n\t"
            "mov %0, %%gs\n\t"
            "mov $0, %%eax\n\t"
            
            /* 压栈：返回地址、CS、EFLAGS、ESP、SS */
            "push %2\n\t"
            "push %1\n\t"
            "push %3\n\t"
            "push %4\n\t"
            "push %5\n\t"

            /* 执行 IRET，返回用户模式 */
            "iret\n\t"
            :
            : "r"(user_ds), "r"(stack_top), "r"(user_ds), "r"(eflags), "r"(user_cs), "r"(entry)
            : "memory"
    );

    /* 不会执行到这里 */
    return;
}

static void copy_open_files(task_t *child)
{
    task_t *parent = current;

    for (int i=0; i<TASK_OPEN_MAX_FILES; i++) {
        file_t *file = parent->open_file_table[i];
        if (file) {
            file_inc_ref(file);
            child->open_file_table[i] = parent->open_file_table[i];
        }
    }
}

static void distory_memory(page_dir_t *pde)
{
    int index = pde_index(USER_MEMORY_BEGIN);
    for (int i=index; i<PAGE_SIZE/4; i++) {
        if (!pde[i].present) {
            continue;
        }

        page_table_t *pte = (page_table_t*)pde[i].value;
        for (int j=0; i<PAGE_SIZE/4; i++) {
            if (!pte[i].present) {
                continue;
            }

            if (pte[i].value != 0) {
                free_pages(pte[i].value, 1);
            }
        }
    }

}

static int copy_memory(page_dir_t *pde)
{
    /* 父进程的页表目录地址 */
    page_dir_t *ppde = (page_dir_t*)current->cr3;

    int index = pde_index(USER_MEMORY_BEGIN);
    for (int i=index; i<PAGE_SIZE/4; i++) {
        if (!ppde[i].present) {
            continue;
        }

        uint page = alloc_pages(1);
        if (!page) {
            distory_memory(pde);
            return -1;
        }

        pde[i].value = page | PAGE_SHARED;

        page_table_t *ppte = (page_table_t*)align_down(ppde[i].value, PAGE_SIZE);
        page_table_t *pte = (page_table_t*)page;
        for (int j=0; j<PAGE_SIZE/4; j++) {
            if (!ppte[j].present) {
                continue;
            }

            uint pg = alloc_pages(1);
            if (!pg) {
                distory_memory(pde);
                return -1;
            }

            pte[j].value = pg | PAGE_SHARED;

            /* 拷贝数据 */
            kmemcpy((void*)pg, (const void*)align_down(ppte[j].value, PAGE_SIZE), PAGE_SIZE);
        }
    }

    return 0;
}

static int do_fork()
{
    uint eip = 0;
    uint esp = 0;
    uint entry= 0;

    /* 取用户空间进入中断指令的后一条指定地址和用户栈指针，以及调用本函数后的内核空间地址 */
    __asm__ __volatile__(
            "mov 44(%%esp), %0\n\t"
            "mov 188(%%esp), %1\n\t"
            "mov 200(%%esp), %2\n\t"
            :"=r"(eip), "=r"(entry), "=r"(esp)
    );

    task_t *parent = current;

    /* 申请一个新的进程结构体 */
    task_t *child = alloc_task();
    if (child < 0) {
        return -1;
    }

    task_init(child, parent->name, eip);
    child->tss.eip = entry;
    child->tss.esp = esp;

    copy_open_files(child);
    child->parent = parent;

    /* 拷贝内存空间内容，这边没有实现写时复制，因为我们的功能能实现就OK，主要是了解内核原理 */
    int err = copy_memory((page_dir_t *)child->cr3);
    if (err < 0) {
        /* 设置为ZOMBIE状态，空闲进程回收 */
        child->state = TASK_ZOMBIE;
        return -1;
    }

    child->state = TASK_RUNNING;
    return child->pid;
}

int sys_fork()
{
    int pid = do_fork();

    /* 子进程，返回用户空间 */
    if (pid == 0) {
        switch_to_user_mode(current->tss.eip, current->tss.esp);
    }

    return pid;
}


/* 从文件中加载进程 */
int sys_execve(char *name, char **argv, char **env)
{
    task_t *task = current;

    /* execve是替换当前进程，所以程序名称需要修改 */
    kstrcpy(task->name, GET_FILENAME(name));

    uint o_page_dir = task->cr3;
    /* 在内核中各进程共享相同的地址映射，可以直接跨进程创建PDE */
    uint n_page_dir = create_pde();
    if (n_page_dir <= 0) {
        return -1;
    }

    uint entry = load_elf_file(name, n_page_dir);
    if (entry <= 0) {
        destroy_pde(n_page_dir);
        return -1;
    }

    /* 准备用户栈 */
    int err = alloc_vpages((page_dir_t*)n_page_dir, 0xFFFFFFFF - USER_STACK_SIZE, USER_STACK_SIZE + 4096);
    if (err < 0) {
        destroy_pde(n_page_dir);
        return -1;
    }

    /* 复制参数和环境变量 */
    uint stack_top = copy_args_envs((page_dir_t*)n_page_dir, argv, env);
    if (stack_top < 0) {
        destroy_pde(n_page_dir);
        return -1;
    }

    destroy_pde(o_page_dir);

    /* 切换页表 */ 
    task->cr3 = n_page_dir;
    write_cr3(n_page_dir);

    current->tss.eip =entry;
    current->tss.esp = stack_top;

    return 0;
}

