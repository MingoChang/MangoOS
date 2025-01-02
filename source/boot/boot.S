### ********************
### auther: Mingo
### date: 2024-08-20
### ********************

### 该程序跑在16位实模式下
    .code16
    .global _start

_start:
    xor %ax, %ax
    mov %ax, %ds    # 设置ds寄存器为0x0000
    mov %ax, %es    # 设置es寄存器为0x0000

    mov $0x7c00, %sp    # 设置栈顶，链接的时候会把本程序加载到0x7c00的位置，栈往低地址增长

    mov $0x0003, %ax # 清空屏幕
    int $0x10

    mov %cs, %ax
    mov %ax, %ds

	/* 取分区起始扇区低8位 */
	mov 0x7dc6, %eax
	mov %ax, part_start
	/* 取分区起始扇区高16位 */
	shr $16, %eax
	mov %ax, params + 10

	call loader

	ljmp $0x00, $0x8000	# 跳转到loader程序开始执

    jmp .           # 程序在此无限循环

loader:
    pusha

    mov $0x8000, %si    # 读超级块内容到内存0x8000处
    mov $0x0002, %ax    # 从磁盘偏移1024处读Ext2超级块内容，后面磁盘驱动那边对磁盘进行了分区，从第2048扇区开始
    mov $0, %di         # 从磁盘偏移1024处读Ext2超级块内容
    mov $0x0001, %cx    # 读1个扇区
    call read_disk

    mov 0x18(%si), %cl      # 读取块大小左移动指数，这个数是一个4字节的数，作为指数，1个字节存储应该就够了，不
                            # 知道为什么搞这么大，我们这边只取一个字节
    mov $1024, %ax
    shlw %cl, %ax           # 1024左移这个指数就是块大小
    mov %ax, block_size     # 保存块大小

    mov 0x58(%si), %bx
    mov %bx, inode_size     # 保存inode结构大小

    mov $0x8000, %si        # 读块组描述符表到内存0x8000处
    shr $0x09, %ax          # ax中还存储这块大小，这个块大小右移动9位，相当于除以512的扇区大小，得到每块有多少个扇区
                            # Ext2文件系统中，块组描述符位于超级块的后面一个块，起始扇区数刚好就是每块扇区数
    mov %ax, sector_num     # 保存每块扇区数
    mov %ax, %cx            # 读一个块
    call read_disk

    mov sector_num, %dx     # 每块扇区数作为被乘数
    mov 8(%si), %ax         # 取得inode表的起始块
    imul %dx                # 乘后结果存储在ax中
    mov %ax, inode_block    # 存储inode表的起始扇区

    mov $2, %ax             # 读根目录内容，Ext2根目录indoe ID为2
    call read_blocks

	push $dir_name			# 目录名boot的地址入栈
	push $4					# 目录名boot的长度入栈
    call find_inode
	add $4, %sp				# 平衡栈
    cmp $0, %bx             # 返回的inode节点ID为0，说明读取失败
    je load_fail
	
    mov %bx, %ax             # 读boot目录内容
    call read_blocks
	push $file_name			# 文件名loader的地址入栈
	push $6					# 文件名loader的长度入栈
    call find_inode
	add $4, %sp				# 平衡栈
    cmp $0, %bx             # 返回的inode节点ID为0，说明读取失败
    je load_fail

    mov %bx, %ax            # 读取loader的文件内容到0x8000内存出
    call read_blocks

    jmp loader_ret

load_fail:
    lea file_not_found, %si
    call puts
    
loader_ret:
	popa
    ret

### 查找文件，文件名起始地址和长度通过栈传入，找到的文件inode ID通过bx返回
find_inode:
    mov $0x8000, %di
	mov %sp, %bp	# 获取栈基址

search_loop:
    mov 0(%di), %ax # 读取inode号
    cmp $0, %ax     # 如果为0，表示遍历完毕
    je not_found

    mov 6(%di), %cl # 读取文件名长度
    cmp 2(%bp), %cl
    jne next_dir    # 文件名长度不相等，则跳到下一个目录项

    mov 4(%bp) , %si
    mov %di, %bx
    add $8, %bx		# 当前目录下中的文件名开始位置

    call compare
    cmp $0, %al     # 比较返回式，如果al寄存器清零0，则找到，否则没有找到
    jne not_found

    mov 0(%di), %bx # 将文件的inode号存储在bx寄存器中
    jmp find_ret

next_dir:
    xor %ax, %ax
    mov 4(%di), %al # 获取当前目录项大小
    add %ax, %di    # 跳到下一项目录首地址
    jmp search_loop

not_found:
    xor %bx, %bx

find_ret:
    ret

### 比较两个字符串是否相等, 字符串地址存贮在bx开的的地址
compare:
	cld				# DF标志位清零，lodsb指令增加模式
	mov 2(%bp), %cx
	
compare_loop:
	lodsb
	cmpb (%bx), %al
	jne not_equal
	inc %bx
	loop compare_loop

	xor %ax, %ax
	jmp compare_ret
	
not_equal:
	mov $1, %ax		# 不相等，返回1
compare_ret:
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
	mov %dx, %di			# 乘积的高16位存储在di中
    add inode_block, %ax    # 取到当前要读的inode的项所在的inode表的第几页的起始扇区号

    mov $0x8000, %si        # 读inode表内容到内存0x8000处
    mov sector_num, %cx     # 读1个块,根目录的inode位于inode表的第2项
    call read_disk

    mov inode_size, %ax
	pop %cx					# 取前面保存的当前块第几项位置
    imul %cx                # inode大小乘以本页inode表中的第几项，得到inode在块内的偏移，存放在ax中
	mov %dx, %di			# 乘积的高16位存储在di中
    add %ax, %si
                            # inode结构偏移0x28的地方是该inode节点数据块号数组的起始位置
    mov 0x28(%si), %ax      # 得到当前inode节点的数据块号
    mov 0x1c(%si), %cx      # 得到该inode节点对应文件占的扇区数

    mov sector_num, %dx     # 每块扇区数作为被乘数
    imul %dx                # 乘后结果存储在ax中，作为读磁盘的起始扇区
	mov %dx, %di			# 乘积的高16位存储在di中

    mov $0x8000, %si        # 读inode节点的数据块到内存0x8000处
    call read_disk

    popa
    ret

### 读磁盘函数，起始扇区低16位存入ax，高16位存入di，读取扇区数cx，读到段寄存器es，段内偏移si 
### 起始扇区是64位的，这边只考虑32位情况，简单起见
read_disk:
    pusha

	add part_start, %ax
read:
    mov %es, params + 6     # 读到内存的位置段地址
    mov %si, params + 4     # 读取到段内偏移地址
    mov %ax, params + 8     # 开始读的扇区

    call read_sec

    add $512, %si
    add $1, %ax

    loop read

    popa
    ret

read_sec:
    pusha

read_loop:
    mov $0x42, %ah  # INT13扩展读功能
    mov $0x80, %dl  # 选择第一个硬盘
    lea params, %si # DS:SI指向参数结构体
    int $0x13

    jc read_loop    # CF置位，说明读取失败，无限尝试

    popa
    ret

puts:                                   
    push %ax                            
next:                                   
    mov %ds:(%si), %al                  
    inc %si                             
    test %al, %al   # 判断是否字符串结尾
    jz 2f                               
    cmp $'\n', %al                      
    jz 1f                               
                                        
    call putc                           
    jmp next                            
                                        
1:  mov $'\r', %al  # 调用回车          
    call putc                           
    mov $'\n', %al  # 调用换行          
    call putc                           
2:                                      
    pop %ax                             
    ret                                 

putc:
    pusha

    xor %bh, %bh    # 第0页屏幕
    mov $0x0e, %ah  # 显示字符，光标自动移动
    int $0x10

    popa
    ret

	file_not_found: .string "File not found\n"
	dir_name:		.asciz "boot"
	file_name:		.asciz "loader"

	block_size: .word 0		# 块大小
	inode_size: .word 0		# inode结构大小
	sector_num:	.word 0		# 每个块的扇区数
	inode_block:.word 0		# inode表的起始扇区

	part_start: .word 0x0000	# 分区开始扇区号

	params:                                     
		.word 0x0010        # 参数表大小为16字节
		.word 0x0001        # 要读取的扇区数    
		.word 0x8000        # 缓冲区偏移量      
		.word 0x0000        # 缓冲区段地址(ES)  
		.word 0x0000        # 开始扇区的(0-15)  
		.word 0x0000        # 开始扇区的(16-31) 
		.word 0x0000        # 开始扇区的(32-47) 
		.word 0x0000        # 开始扇区的(48-63) 
    
    .org 510
    .word 0xaa55 
