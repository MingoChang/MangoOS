### ********************
### auther: Mingo
### date: 2024-08-20
### ********************

### 定义GDT描述符结构体
.macro Descripter base, limit, attribute
    .word   (\limit) & 0xffff                     						# 限长低 16 位
    .word   (\base) & 0xffff                      						# 基址低 16 位
    .byte   ((\base >> 16) & 0xff)                						# 基址中 8 位
    .word   (((\limit >> 16) & 0x0f) << 8) | ((\attribute) & 0xf0ff)  	# 限长高 4 位和属性
    .byte   ((\base >> 24) & 0xff)                						# 基址高 8 位
.endm
