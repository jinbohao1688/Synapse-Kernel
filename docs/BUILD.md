# Building Synapse Kernel

This guide provides detailed instructions on how to build the Synapse Kernel on different platforms.

## Prerequisites

### Required Tools

- **Cross-compilation toolchain**: `i686-elf-gcc` and related tools
- **Assembly**: `nasm`
- **Linker**: `i686-elf-ld`
- **Make**: GNU Make
- **QEMU** (for testing): `qemu-system-i386`

### Platform-specific Setup

#### Windows (MSYS2)

1. **Install MSYS2** from [msys2.org](https://www.msys2.org/)
2. **Update MSYS2 packages** in the UCRT64 terminal:
   ```bash
   pacman -Syu
   ```
3. **Install required packages**:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-nasm mingw-w64-ucrt-x86_64-gcc make qemu-system-i386
   ```
4. **Download pre-built i686-elf toolchain** from [i686-elf-tools](https://github.com/lordmilko/i686-elf-tools/releases)
5. **Extract the toolchain** to a directory like `C:\i686-elf-tools`
6. **Add the toolchain to PATH**:
   ```bash
   export PATH="C:\i686-elf-tools\bin:$PATH"
   ```

#### Linux (Ubuntu/Debian)

1. **Update package lists**:
   ```bash
   sudo apt-get update
   ```
2. **Install required packages**:
   ```bash
   sudo apt-get install build-essential nasm qemu-system-x86
   ```
3. **Build i686-elf toolchain from source** (recommended):
   - Follow the [OSDev Wiki guide](https://wiki.osdev.org/GCC_Cross-Compiler)

## Building the Kernel

### Using the Makefile

1. **Navigate to the project root directory**:
   ```bash
   cd /path/to/synapse-kernel
   ```

2. **Set the cross-compilation prefix**:
   ```bash
   export CROSS_COMPILE=i686-elf-
   ```

3. **Build the kernel**:
   ```bash
   make
   ```

4. **The kernel binary** will be generated as `synapse.bin` in the project root.

### Building Manually

If you prefer to build manually, use the following commands:

```bash
# Assemble boot sector
nasm -f elf32 kernel/boot/boot.asm -o kernel/boot/boot.o

# Compile kernel sources
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Ikernel/include -c kernel/main.c -o kernel/main.o
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Ikernel/include -c kernel/mm/kheap.c -o kernel/mm/kheap.o
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Ikernel/include -c kernel/proc/process.c -o kernel/proc/process.o
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Ikernel/include -c kernel/fs/vfs.c -o kernel/fs/vfs.o
i686-elf-gcc -ffreestanding -nostdlib -O2 -Wall -Wextra -std=gnu99 -Ikernel/include -c kernel/loader/elf.c -o kernel/loader/elf.o

# Link all objects
i686-elf-ld -T linker.ld -nostdlib -o synapse.bin kernel/boot/boot.o kernel/main.o kernel/mm/kheap.o kernel/proc/process.o kernel/fs/vfs.o kernel/loader/elf.o
```

## Building User Applications

1. **Navigate to the apps directory**:
   ```bash
   cd apps
   ```

2. **Run the build script**:
   ```bash
   ./build.sh
   ```

3. **The compiled binaries** will be in the `apps/bin` directory.

## Running the Kernel

### Using QEMU

1. **Ensure QEMU is installed**
2. **Run the kernel** using the provided script:
   ```bash
   cd tools
   ./qemu-run.sh
   ```

### Using the Manual QEMU Command

```bash
qemu-system-i386 -kernel synapse.bin -m 128M -monitor stdio -serial COM1
```

## Debugging

### Using QEMU Debug Output

```bash
qemu-system-i386 -kernel synapse.bin -m 128M -monitor stdio -serial COM1 -d int -no-reboot
```

### Using GDB

```bash
# In one terminal
eqemu-system-i386 -kernel synapse.bin -m 128M -monitor stdio -serial COM1 -s -S

# In another terminal
gdb synapse.bin -ex "target remote localhost:1234" -ex "break main" -ex "continue"
```

## Troubleshooting

### Common Issues

1. **"i686-elf-gcc: command not found"**
   - Solution: Ensure the i686-elf toolchain is in your PATH

2. **"undefined reference to..." during linking**
   - Solution: Check that all required object files are included in the linker command

3. **QEMU fails to start**
   - Solution: Ensure QEMU is installed and the kernel binary exists

4. **Build fails with "permission denied"**
   - Solution: Ensure all build scripts have execute permissions: `chmod +x build.sh tools/qemu-run.sh`

### Reporting Issues

If you encounter issues during building, please:

1. Check this guide again for any missed steps
2. Verify your toolchain setup
3. Check the kernel/TODO.md file for known issues
4. Create a GitHub issue with:
   - Your operating system
   - Toolchain version
   - Exact error message
   - Steps to reproduce

## Additional Information

- **Kernel Architecture**: 32-bit x86
- **Build System**: GNU Make
- **Supported Platforms**: QEMU, VirtualBox, physical x86 hardware
- **License**: MIT

For more information, see the [README.md](../README.md) and [SYSCALLS.md](./SYSCALLS.md) files.
