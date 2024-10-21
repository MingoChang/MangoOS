TARGETS = boot.bin loader
OBJECTS = boot/boot.o boot/loader.o
ELFS = boot.elf loader.elf
SOURCE_FILES = boot/boot.S boot/loader.S
BOOT_SRC = boot
DISK_NAME = ../image/system.img
MOUNT_DIR = ../image/system

CC = gcc
LD = ld
CFLAGS = -g -c -O0 -m32 -nostartfiles -no-pie -fno-stack-protector -nostdlib -nostdinc
LDFLAGS = -m elf_i386

all:$(TARGETS)

$(BOOT_SRC)/%.o: $(BOOT_SRC)/%.S
	$(CC) $(CFLAGS) -o $@ $<

boot.elf: boot/boot.o
	$(LD) $(LDFLAGS) -Ttext 0x7c00 -o $@ $<
loader.elf: boot/loader.o
	$(LD) $(LDFLAGS) -Ttext 0x8000 -o $@ $<

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
	@sync
	@sudo umount $(MOUNT_DIR)

clean:
	rm -rf $(OBJECTS) $(TARGETS) $(ELFS)
