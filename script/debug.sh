#!/bin/bash
location=$(dirname "$0")
# 启动 QEMU，并保存进程 ID 到 qemu.pid 文件
qemu-system-i386 \
    -daemonize -m 256M \
    -s -S \
    -drive file=$location/../image/system.img,index=0,media=disk,format=raw \
    -d pcall,page,mmu,cpu_reset,guest_errors,page,trace:ps2_keyboard_set_translation

# 给 QEMU 500 毫秒时间启动
sleep 0.5

# 启动 GDB
gdb $location/../source/boot.elf \
    -ex "set confirm off" \
    -ex "add-symbol-file $location/../source/loader.elf 0x8000" \
    -ex "add-symbol-file $location/../source/kernel.elf 0x20000" \
    -ex "target remote :1234" \
    -ex "b _start" \
    -ex "continue"

# 关闭QEMU
PID=$(pgrep -f "qemu-system-i386")
echo "$PID" | xargs kill -9
