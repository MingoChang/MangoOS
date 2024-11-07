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
