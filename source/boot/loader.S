### ********************
### auther: Mingo
### date: 2024-09-02
### ********************

.include "boot/head.S"

### 16位代码段
.section .text
    .code16
    .global _start

_start:
    xor %ax, %ax
    mov %ax, %ds    # 设置ds寄存器为0x0000
    mov %ax, %es    # 设置es寄存器为0x0000
	mov %ax, %ss	# 设置栈寄存器

	mov $0x8000, %sp # 设置栈顶

	lea loader, %si	# 取字符串地址
	call puts

	call get_hardware_msg # 获取内存，显卡等硬件信息

    call switch_to_protected_mode  # 切换到保护模式

	jmp .

### 使用显存方式打印数据到屏幕
puts:
	pusha
    mov $0xB800, %ax	# 将显存段地址 0xB800 加载到 AX
    mov %ax, %es		# 将 AX 的值加载到 ES，ES 现在指向显存区域
	mov position, %di	# 取当前显示位置到bx中

display_loop:
	lodsb				# 加载ds:si的字符到al中
    mov $0x07, %ah		# 0x07 是亮白色字符, 黑色背景
    mov %ax, %es:(%di)	# 将 AX 的值写入 ES:bx
	test %al, %al		# 到达字符串末尾
	jz puts_ret

	add $2, %di			# 一个字符包含显示属性，占2字节
	jmp display_loop

puts_ret:
	addw $160, position	# 下一行
	popa
	ret

get_hardware_msg:
	pusha
	
    xor %ebx, %ebx           # EBX 为 0，表示开始获取第一块内存区域
    mov $0x534D4150, %edx    # "SMAP" 魔术数 ("SMAP" 的 ASCII 表示)
    mov $20, %ecx            # 每个内存块描述符大小为 20 字节
	mov $0x1000, %ax
    mov %ax, %es             # 设置 ES 段为 0x1000，存储信息的段地址
    xor %di, %di             # 偏移为 0 (指向 0x10000 处)

get_memory_loop:
    mov $0xE820, %eax        # 功能号 0xE820，获取内存布局
    int $0x15                # 调用 BIOS 中断

    jc done                  # 如果出错，则跳转到 done
	incw mem_sum			 # 没有出错，就增加内存块计数
	mov %es:8(%di), %eax	 # 取该段内存大小
	add %eax, mem_size		 # 增加内存大小，目前只考虑32位能表示的内存大小
    cmp $0, %ebx             # 检查是否有更多的内存区域
    je done                  # 如果 EBX 返回 0，跳转到 done

    add $20, %di             # 每个内存块描述符占 20 字节，移动 DI 指针
    jmp get_memory_loop      # 获取下一块内存区域

done:
	popa
	ret

### 切换到保护模式
switch_to_protected_mode:
    cli                 # 关中断

### 开启A20地址线
	inb $0x92, %al
	orb $0x02, %al
	outb %al, $0x92

    lgdt gdt_descriptor  # 加载GDT

    mov %cr0, %eax       # 读取CR0
    or $0x1, %eax        # 设置保护模式位 (PE 位为1)
    mov %eax, %cr0       # 写回CR0，启用保护模式

    jmp $SelectorCode, $protected_mode_entry # 远跳转到32位代码段

### 32位代码段
.section .text
    .code32

protected_mode_entry:
    mov $SelectorData, %ax 	# 保护模式数据段选择子
    mov %ax, %ds         	# 设置ds段寄存器
    mov %ax, %es         	# 设置es段寄存器
    mov %ax, %fs         	# 设置fs段寄存器
    mov %ax, %ss         	# 设置ss段寄存器
    mov $SelectorVideo, %ax # 保护模式数据段选择子
    mov %ax, %gs         	# 设置gs段寄存器

    mov $0x8000, %esp    	# 重新设置32位栈

    lea pro_mode, %esi   	# 显示32位模式下的消息
    call puts32

	call load_kernel		# 加载内核到内存中
	call display_mem		# 显示内存布局信息
	call setup_page			# 启用分页
	call place_kernel		# 按elf格式放置内核在内存中

    jmp $SelectorCode, $KernelEntryPoint # 远跳转到内核代码

    jmp .

### 分页
setup_page:
	pusha

	xor %edx, %edx		# 只考虑32位表示的内存大小
	mov mem_size, %eax	# 内存大小
	mov $0x400000, %ebx # 4M = 也表对应的内存大小
	div %ebx
	mov %eax, %ecx		# 存储内存需要多少个页目录
	test %edx, %edx
	jz 1f
	inc %ecx			# 有余数，则增加一个页目录

1:
	push %ecx			# 暂存页目录数

	mov $PageDirBase, %edi
	xor %eax, %eax
	mov $PageTableBase + 7, %eax	# 0x07,表示该页存在,可读写

dir_loop:
	stosl				# 将eax的内容放置到edi指向的内存，然后edi增加4k
	add $4096, %eax		# 下一页表项
	loop dir_loop

	pop %eax
	mov $1024, %ebx
	mul %ebx			# 页表个数 * 1024 = 总共要映射的内存页
	mov %eax, %ecx
	mov $PageTableBase, %edi
	xor %eax, %eax
	mov $7, %eax		# 0x07，表示该页对应的内存存在可读写
	
table_loop:
	stosl				# 初始化各页表内容
	add $4096, %eax		# 下一内存块
	loop table_loop

	mov $PageDirBase, %eax
	mov %eax, %cr3		# 开启分页机制
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0

	popa
	ret

### 按ELF格式放置内核到内存中
place_kernel:
	pusha
	mov $0x20000, %edi	# 目前内核存放的地址
	xor %esi, %esi

	mov 0x2c(%edi), %cx		# Program header table中有多少条记录
	movzx %cx, %ecx			# 高位补0
	mov 0x1c(%edi), %esi	# Pragram header table在文件中的偏移
	add %edi, %esi			# Pragram header table在现在内存中的位置 

place_loop:
	mov 0x10(%esi), %eax	# 段长度
	cmp $0, %eax
	je 1f

	push %eax				# memcpy的参数--拷贝长度
	mov 0x04(%esi), %eax	# 段起始位置
	add %edi, %eax			# 段起始位置在内存中的位置
	push %eax				# memcpy的参数--源地址
	mov 0x08(%esi), %eax
	push %eax				# memcpy的擦数--目的地址

	call memcpy
	add $12, %esp			# 栈平衡
1:
	add $0x20, %esi			# 下一个Pragram header项
	loop place_loop

	popa
	ret

### 内存拷贝
memcpy:
	mov %esp, %ebp			# 获取栈顶地址
	pusha

	mov 4(%ebp), %edi		# 目的地址
	mov 8(%ebp), %esi		# 源地址
	mov 12(%ebp), %ecx		# 长度

memcpy_loop:
	mov (%esi), %al
	mov %al, (%edi)
	inc %esi
	inc %edi
	loop memcpy_loop

	popa
	ret

### 使用显存方式打印数据到屏幕
puts32:
	pusha
	mov position, %di	# 取当前显示位置到bx中

1:
	lodsb				# 加载ds:si的字符到al中
    mov $0x07, %ah		# 0x07 是亮白色字符, 黑色背景
    mov %ax, %gs:(%di)	# 将 AX 的值写入 ES:bx
	test %al, %al		# 到达字符串末尾
	jz 1f

	add $2, %di			# 一个字符包含显示属性，占2字节
	jmp 1b

1:
	addw $160, position	# 下一行
	popa
	ret

### 显示内存信息
display_mem:
	pusha

	mov mem_sum, %cx	# 总共的内存块数
	mov $0x10000, %esi	# 从BIOS读取到的内存布局存放地址
	xor %edi, %edi
display_mem_loop:
	mov position, %di	# 当前显存位置
	mov $5, %bx
1:
	mov (%esi), %eax
	call display_eax
	add $4, %esi
	dec %bx
	cmp $0, %bx
	ja 1b

	addw $160, position		# 下一行
	loop display_mem_loop

	popa
	ret

### 在显存中以16进制的方式显示一个字的数字
display_eax:
	push %eax
	push %ebx

	mov %eax, %ebx
	shr $24, %eax			# 高字节
	and $0xff, %al
	call display_al

	mov %ebx, %eax
	shr $16, %eax			# 次高字节
	and $0xff, %al
	call display_al
	
	mov %ebx, %eax
	shr $8, %eax			# 次低字节
	and $0xff, %al
	call display_al

	mov %ebx, %eax
	and $0xff, %al			# 低字节
	call display_al

	mov $0x07, %ah			# 黑色背景，白色字符
	mov $' ', %al
	mov %al, %gs:(%edi)		# 打印一个空格
	add $2, %edi

	pop %ebx
	pop %eax
	ret

### 在显存中以16进制的方式显示一个字节的数字
display_al:
	push %ecx
	push %edx

	mov $0x07, %ah			# 黑色背景，白色字符
	mov %al, %dl			# 低4位放在DL中
	shr $4, %al				# 高4位放在AL中
	mov $2, %ecx			# 分两次输出

dispaly_al_loop:
	and $0xf, %al			# AL已经右移动了4位，高4位清零
	cmp $9, %al				
	ja 1f					# 数字大等于10
	add $'0', %al
	jmp 2f	
1:
	sub $0xa, %al			# 只保留大等于10的部分
	add $'A', %al			# 加上字符'A'的编码
2:
	mov %al, %gs:(%edi)		# 拷贝到显存中，从而在屏幕上显示
	add $2, %edi			# 2个字节表示一个字符

	mov %dl, %al			# 打印低4位表示的字符
	loop dispaly_al_loop
	
	pop %edx
	pop %ecx
	ret
	
### 加载内核
load_kernel:
	pusha

	mov $1, %cx
	mov $0x20000, %edi
	mov $0x0002, %ebx
	call read_disk		# 读取Ext2超级块到内存0x20000处

	mov 0x18(%edi), %cl
	mov $1024, %ax
	shlw %cl, %ax
	mov %ax, block_size	# 从超级快0x18偏移的位置读取块大小指数，1024左移这个指数就是块大小	

	shl $2, %cl			# 块大小指数*2就是每块扇区数
	mov %cl, sector_num # 保存每块扇区数
	
	mov 0x58(%edi), %bx
	mov %bx, inode_size	# 保存inode结构体大小

	mov %ecx, %ebx		# cl中存储每块扇区数，块组描述符在第二个块中，所以起始扇区就是等于每块扇区数
	call read_disk		# 读取块组描述符到内存0x20000处，参数都已经在前面的几个步骤中设置好了

	mov 0x8(%edi), %ax	# 取inode表的起始块
	imul %cx			# 乘每块扇区数
	mov %ax, start_block # 存储inode表的起始扇区

	mov $2, %ax			# 根目录inode ID
	call read_blocks	# 读根目录内容

	push $dir_name		# 查找boot目录
	push $4
	call find_inode
	add $8, %esp		# 栈平衡
	cmp $0, %eax
	je load_fail

	call read_blocks	# 读取boot目录下的文件名
	push $file_name
	push $6
	call find_inode
	add $8, %esp
	cmp $0, %eax
	je load_fail

	call read_blocks	# 读取内核到内存中

load_fail:
	popa
	ret

### 比较两个字符串是否相等，对比的字符串存储在ebx中
compare:
	cld
	mov 4(%ebp), %ecx	# 从栈中取得字符串长度
compare_loop:
	lodsb
	cmp (%ebx), %al
	jne not_equal
	add $1, %ebx
	loop compare_loop

	xor %eax, %eax
	jmp compare_ret

not_equal:
	mov $1, %eax
compare_ret:
	ret

### 查找文件，文件名起始地址和长度通过栈传入，找到的文件的inode ID通过eax返回
find_inode:
	mov $0x20000, %edi	
	mov %esp, %ebp		# 获取栈基址

search_loop:
	mov 0(%edi), %eax	# 读取文件的inode ID
	cmp $0, %eax		# 如果为0， 表示遍历完毕
	je not_found

	mov 6(%edi), %cl	# 读取文件名长度
	cmp 4(%ebp), %cl
	jne	next_dir		# 文件名长度都不相等，查找下一目录项

	mov 8(%ebp), %esi	# 传入的文件名位置
	mov %edi, %ebx
	add $8, %ebx		# 当前目录文件名开始的位置
	call compare
	cmp $0, %eax
	jne next_dir

	mov 0(%edi), %eax	# 找到，取inode ID返回
	jmp ret

next_dir:
	mov 4(%edi), %ax	# 获取当前目录项大小
	add %eax, %edi		# 从下一个目录项继续查找
	jmp search_loop

not_found:
	xor %eax, %eax

ret:
	ret

### 读取inode号对应的块到内存, ax中传递inode号
read_blocks:
    pusha

    push %ax                # 先保存inode的ID，一会要用

    xor %dx, %dx            # 16位除法，被除数是32位的，高16位在dx中
    mov block_size, %ax
    mov inode_size, %bx
    div %bx                 # 块大小/inode结构大小 = 一个块可以放几个inode表项，放在ax中

    mov %ax, %bx            # 每块可以放inode数作为除数
    pop %ax
    sub $1, %ax             # 当前inode ID-1作为被除数
    div %bx                 # 商ax中存inode表的第几块，余数dx是在这个块中的第几项
    push %dx	            # 保存当前块第几项位置，后面要用

    mov sector_num, %bx
    imul %bx                # 块数乘以每块扇区数，得到多少个扇区，放在ax里面
	mov %dx, %bx			# 乘积的高16位存储在ebx高16位中
	shl $16, %ebx
    add start_block, %ax    # 取到当前要读的inode的项所在的inode表的第几页的起始扇区号
	mov %ax, %bx

    mov $0x20000, %edi      # 读inode表内容到内存0x20000处
    mov sector_num, %cx     # 读1个块,根目录的inode位于inode表的第2项
    call read_disk

    mov inode_size, %ax
	pop %cx					# 取前面保存的当前块第几项位置
    imul %cx                # inode大小乘以本页inode表中的第几项，得到inode在块内的偏移，存放在ax中
	push %ax				# 存储乘积的低16位
	mov %dx, %ax			# 乘积的高16位存储在eax的高16位中
	shl $16, %eax			 
	pop %ax
    add %eax, %edi
                            # inode结构偏移0x28的地方是该inode节点数据块号数组的起始位置
    mov 0x28(%edi), %ax     # 得到当前inode节点的数据块号
    mov 0x1c(%edi), %cx     # 得到该inode节点对应文件占的扇区数

    mov sector_num, %dx     # 每块扇区数作为被乘数
    imul %dx                # 乘后结果存储在ax中，作为读磁盘的起始扇区
	mov %dx, %bx			# 乘积的高16位存储在ebx的高16位中
	shl $16, %ebx
	mov %ax, %bx

    mov $0x20000, %edi      # 读inode节点的数据块到内存0x20000处
    call read_disk

    popa
    ret


### 读磁盘，cx中传递读取扇区数，ebx传递起始扇区地址, edi指向的地址存储读取到的内容
read_disk:
	pusha

read:
	call read_sec
	add $512, %edi
	add $1, %ebx
	loop read

	popa
	ret

### 读扇区，起始地址防止在ebx中， edi指向的地址存储读取到的内容
read_sec:
	pusha

	mov $0x1F6, %dx
	mov $0xE0, %al
	outb %al, %dx		# 选择驱动器号（0号硬盘）

	mov $0, %al
	mov $0x1F2, %dx
	outb %al, %dx		# 读取扇区数高字节
	
	mov %ebx, %eax
	shr $24, %eax
	mov $0x1F3, %dx
	outb %al, %dx		# 起始扇区的24-31位
	
	mov $0, %al
	mov $0x1F4, %dx
	out %al, %dx		# 起始扇区的32-39位
	mov $0x1F5, %dx
	outb %al, %dx		# 起始扇区的40-47位

	mov $1, %al
	mov $0x1F2, %dx
	outb %al, %dx		# 读取扇区数低字节
	
	mov %ebx, %eax
	mov $0x1F3, %dx
	outb %al, %dx		# 起始扇区的0-7位
	
	mov %ebx, %eax
	shr $8, %eax
	mov $0x1F4, %dx
	outb %al, %dx		# 起始扇区的8-15位
	
	mov %ebx, %eax
	shr $16, %eax
	mov $0x1F5, %dx
	outb %al, %dx		# 起始扇区的16-23位

	mov $0x1F7, %dx
	mov $0x24, %al
	outb %al, %dx		# 发送读指令

wait_for_data:
	inb %dx, %al		# 读取状态
	and $0x88, %al
	test $0x8, %al		# 数据就绪位
	jz wait_for_data

	mov $0x1F0, %dx		# 数据端口
	mov $256, %cx       # 因为读取磁盘一次读2个字节

read_data:
	inw %dx, %ax
	mov %ax, (%edi)		# 存在在edi指向的地址中
	add $2, %edi
	loop read_data

	popa
	ret

.section .data
	LABEL_GDT:      Descripter 0, 0, 0        # 空描述符
	LABEL_CODE:     Descripter 0, 0xFFFFF, 0xCF9A # 代码段
	LABEL_DATA:     Descripter 0, 0xFFFFF, 0xCF92 # 数据段
	LABEL_VIDEO:    Descripter 0xB8000, 0xFFFFF, 0xCF92 # 数据段

gdt_descriptor:
    .word 0x1F      # GDT 大小 (24字节)
    .long LABEL_GDT # GDT 基址

### 段选择子定义
.equ SelectorCode, 0x08   # 代码段选择子，GDT表中代码段的偏移量
.equ SelectorData, 0x10   # 数据段选择子，GDT表中数据段的偏移量
.equ SelectorVideo, 0x18  # 显存段选择子，GDT表中数据段的偏移量

### 定义也目录和也表地址
.equ PageDirBase, 0x200000		# 页目录位于地址2M处，假设内核大小不超过1.9M
.equ PageTableBase, 0x201000	# 也表位于4K处

.equ KernelEntryPoint, 0x30400	# 内核入口点位置

    position:	.word 160	# 从第二行开始显示
    loader:		.string "Begin loader"
    pro_mode:   .string "In Protected mode"
	block_size:	.word 0		# 块大小
	inode_size:	.word 0		# inode大小
	sector_num:	.word 0		# 每块扇区数
	start_block:.word 0		# inode起始扇区
	dir_name:	.asciz "boot"
	file_name:	.asciz "kernel"
	mem_sum:	.word 0		# 内存分块数
	mem_size:	.long 0		# 内存大小
