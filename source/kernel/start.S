### ********************
### auther: Mingo
### date: 2024-10-25
### ********************
.section .text
	.global _start
	.global add
	.extern kernel_init
_start:
	call kernel_init # 初始化内核
	
	jmp .

add:
	pusha

	jmp .

	popa
	ret
