#!/bin/bash

echo "==================================="
echo "Compiling Synapse Kernel Apps"
echo "==================================="

# 设置交叉编译工具链
CROSS_COMPILE=E:/msys64/ucrt64/bin/i686-elf-
CC=${CROSS_COMPILE}gcc
LD=${CROSS_COMPILE}ld
AS=${CROSS_COMPILE}as

# 设置编译标志
CFLAGS="-ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -I../user-lib/include"
LDFLAGS="-T ../user-lib/linker.ld -nostdlib"

# 创建输出目录
mkdir -p bin

echo "\n[1/2] Compiling hello/hello.c..."
${CC} ${CFLAGS} -c hello/hello.c -o hello/hello.o
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile hello.c"
    exit 1
fi

${LD} ${LDFLAGS} ../user-lib/crt0.o hello/hello.o -o bin/hello
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to link hello"
    exit 1
fi

echo "\n[2/2] Compiling ai_demo/ai_viewer.c..."
${CC} ${CFLAGS} -c ai_demo/ai_viewer.c -o ai_demo/ai_viewer.o
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile ai_viewer.c"
    exit 1
fi

${LD} ${LDFLAGS} ../user-lib/crt0.o ai_demo/ai_viewer.o -o bin/ai_viewer
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to link ai_viewer"
    exit 1
fi

echo "\n==================================="
echo "Build complete!"
echo "Generated binaries:"
ls -lh bin/
echo "==================================="
