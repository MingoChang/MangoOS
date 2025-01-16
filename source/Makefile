TARGETS = boot.bin loader kernel.elf init.elf
OBJECTS = boot/boot.o boot/loader.o
KERNEL_OBJS = kernel/start.o kernel/init.o kernel/arch.o kernel/irq.o kernel/time.o kernel/task.o \
			  kernel/sched.o kernel/ipc.o mm/bitmap.o mm/page.o mm/memory.o device/disk.o device/dev.o \
			  fs/fs.o fs/file.o fs/ext2.o fs/buffer.o
INIT_OBJS = init/start.o init/init.o
LIB_OBJS = lib/log.o lib/string.o lib/queue.o lib/list.o
ELFS = boot.elf loader.elf
BOOT_SRC = boot
KERNEL_SRC = kernel
INIT_SRC = init
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
$(INIT_SRC)/%.o: $(INIT_SRC)/%.S
	$(CC) $(CFLAGS) -o $@ $<
$(INIT_SRC)/%.o: $(INIT_SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<
$(LIB_SRC)/%.o: $(LIB_SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<

boot.elf: boot/boot.o
	$(LD) $(LDFLAGS) -Ttext 0x7c00 -o $@ $<
loader.elf: boot/loader.o
	$(LD) $(LDFLAGS) -Ttext 0x8000 -o $@ $<
kernel.elf: $(KERNEL_OBJS) $(LIB_OBJS)
	$(LD) $(LDFLAGS) -Ttext 0x30400 -o $@ $(KERNEL_OBJS) $(LIB_OBJS)
init.elf: $(INIT_OBJS)
	$(LD) $(LDFLAGS) -Ttext 0x40803400 -o $@ $(INIT_OBJS)

boot.bin: boot.elf
	objcopy -O binary $< $@
loader: loader.elf
	objcopy -O binary $< $@

install: $(TARGETS)
	@echo "写入程序到磁盘文件 $(DISK_NAME)..."
	@dd if=boot.bin of=$(DISK_NAME) bs=446 count=1 conv=notrunc
	@sync
	@sudo losetup -P /dev/loop2 $(DISK_NAME)
	@sudo mount /dev/loop2p1 $(MOUNT_DIR)
	@sudo rm -f $(MOUNT_DIR)/boot/kernel $(MOUNT_DIR)/boot/loader $(MOUNT_DIR)/boot/init
	@sudo cp loader $(MOUNT_DIR)/boot/loader
	@sudo cp kernel.elf $(MOUNT_DIR)/boot/kernel
	@sudo cp init.elf $(MOUNT_DIR)/boot/init
	@sync
	@echo "拷贝文件完成..."
	@sudo umount $(MOUNT_DIR)
	@sudo losetup -d /dev/loop2

clean:
	rm -rf $(OBJECTS) $(KERNEL_OBJS) $(LIB_OBJS) $(TARGETS) $(ELFS) $(INIT_OBJS)
