### ********************
### auther: Mingo
### date: 2024-08-20
### ********************

.include "boot/head.S"

### 16位代码段
	.section .text
    .code16
    .global _start

_start:
    cli					# 关中断

    xor %ax, %ax
    mov %ax, %ds    # 设置ds寄存器为0x0000
    mov %ax, %es    # 设置es寄存器为0x0000

	mov $0x8000, %sp # 设置栈顶

	lea loader, %si	# 取字符串地址
	call puts

	sti					# 开中断
	jmp .

### 使用缓存方式打印数据到屏幕
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
	popa
	ret

	.section .data
	position:	.word 160	# 从第二行开始显示
	loader:		.string "Start loader"
	
	GDT: Descripter 0, 0, 0
