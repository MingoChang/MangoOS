### ********************
### auther: Mingo
### date: 2025-1-5
### ********************
.section .text
	.global _start
	.extern user_init
_start:
	call user_init
	jmp .

