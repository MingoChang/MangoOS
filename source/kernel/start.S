### ********************
### auther: Mingo
### date: 2024-10-25
### ********************
.section .text
	.global _start
	.extern kernel_init
_start:
	call kernel_init
	jmp .

### 定义异常处理宏
.macro exception_handler name num err_code
.section .text
	.global exception_handler_\name
	.extern do_handler_\name
exception_handler_\name:
	.if \err_code == 0	# 传入宏的错误码为0，怎说明CPU不会自动压入错误码的异常，我们压入一个0，使得结构体通用。
		push $0
	.endif
	push $\num

	pusha
	push %ds
	push %es
	push %fs
	push %gs

	push %esp		# 这个很关键，指向最新的栈顶，在函数第一个参数可以定义一个结构体，来获取栈内的值
	call do_handler_\name
	add $4, %esp	# 平栈
 
	pop %gs
	pop %fs
	pop %es
	pop %ds
	popa

	add $8, %esp	# 平错误码和中断编号的栈,CPU有自动压入错误码的情况，CPU不会帮忙出栈，所以这边都是8

	iret	# 从中断中返回
.endm

### 默认异常处理
exception_handler default -1 0
exception_handler divide 0 0
exception_handler debug 1 0
exception_handler nmi 2 0
exception_handler bp 3 0
exception_handler of 4 0
exception_handler br 5 0
exception_handler io 6 0
exception_handler du 7 0
exception_handler df 8 1
exception_handler it 10 1
exception_handler snp 11 1
exception_handler ssf 12 1
exception_handler gp 13 1
exception_handler pf 14 1
exception_handler fe 16 0
exception_handler ac 17 1
exception_handler mc 18 0
exception_handler simd 19 0
exception_handler virtual 20 0
exception_handler control 21 1

exception_handler time 0x20 0
exception_handler ide 0x2E 0
exception_handler syscall 0x80 0
