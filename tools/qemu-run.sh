#!/bin/bash

echo "==================================="
echo "Running Synapse Kernel in QEMU"
echo "==================================="

# 设置QEMU路径
QEMU=qemu-system-i386

# 检查QEMU是否可用
if ! command -v ${QEMU} &> /dev/null; then
    echo "ERROR: QEMU not found. Please install qemu-system-i386"
    exit 1
fi

# 设置内核二进制文件路径
KERNEL_BIN=../synapse.bin

# 检查内核二进制文件是否存在
if [ ! -f ${KERNEL_BIN} ]; then
    echo "ERROR: Kernel binary not found at ${KERNEL_BIN}"
    echo "Please build the kernel first with 'make'"
    exit 1
fi

# 运行QEMU
${QEMU} -kernel ${KERNEL_BIN} -m 128M -monitor stdio -serial COM1

# 可选：如果需要更详细的调试输出，可以使用以下命令
# ${QEMU} -kernel ${KERNEL_BIN} -m 128M -monitor stdio -serial COM1 -d int -no-reboot
