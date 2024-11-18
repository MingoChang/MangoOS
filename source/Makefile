TARGETS = boot.bin loader kernel.elf
OBJECTS = boot/boot.o boot/loader.o
KERNEL_OBJS = kernel/start.o kernel/init.o kernel/arch.o kernel/irq.o kernel/time.o kernel/task.o kernel/sched.o
LIB_OBJS = lib/log.o lib/string.o
ELFS = boot.elf loader.elf
BOOT_SRC = boot
KERNEL_SRC = kernel
LIB_SRC = lib
DISK_NAME = ../image/system.img
MOUNT_DIR = ../image/system

CC = gcc
LD = ld
CFLAGS  =-O0 -g3 -gdwarf-2 -m32 -c -fstrength-reduce -fomit-frame-pointer -fno-stack-protector -fno-builtin
LDFLAGS = -m elf_i386 -z noexecstack

all:$(TARGETS)

$(BOOT_SRC)/%.o: $(BOOT_SRC)/%.S
	$(CC) $(CFLAGS) -o $@ $<

$(KERNEL_SRC)/%.o: $(KERNEL_SRC)/%.S
	$(CC) $(CFLAGS) -o $@ $<
$(KERNEL_SRC)/%.o: $(KERNEL_SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<
$(LIB_SRC)/%.o: $(LIB_SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<

boot.elf: boot/boot.o
	$(LD) $(LDFLAGS) -Ttext 0x7c00 -o $@ $<
loader.elf: boot/loader.o
	$(LD) $(LDFLAGS) -Ttext 0x8000 -o $@ $<
kernel.elf: $(KERNEL_OBJS) $(LIB_OBJS)
	$(LD) $(LDFLAGS) -Ttext 0x30400 -o $@ $(KERNEL_OBJS) $(LIB_OBJS)

boot.bin: boot.elf
	objcopy -O binary $< $@
loader: loader.elf
	objcopy -O binary $< $@

install: $(TARGETS)
	@echo "写入程序到磁盘文件 $(DISK_NAME)..."
	@dd if=boot.bin of=$(DISK_NAME) bs=512 count=1 conv=notrunc
	@sync
	@sudo mount -t ext2 $(DISK_NAME) $(MOUNT_DIR)
	@sudo cp loader $(MOUNT_DIR)/boot/
	@sudo cp kernel.elf $(MOUNT_DIR)/boot/kernel
	@sync
	@sudo umount $(MOUNT_DIR)

clean:
	rm -rf $(OBJECTS) $(KERNEL_OBJS) $(LIB_OBJS) $(TARGETS) $(ELFS)
