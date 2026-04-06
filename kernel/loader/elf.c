#include <loader/elf.h>
#include <fs/vfs.h>
#include <mm/kheap.h>
#include <mm/paging.h>
#include <proc/task.h>
#include <vga.h>
#include <string.h>
#include "../../kernel/embedded_bins.h"

/* 内存镜像读取 */
static ssize_t mem_read(const uint8_t *base, size_t base_len, void *buf, size_t count, size_t offset) {
    if (offset >= base_len) return 0;
    size_t avail = base_len - offset;
    size_t n = (count < avail) ? count : avail;
    memcpy(buf, base + offset, n);
    return (ssize_t)n;
}

// 读取文件内容
static ssize_t read_file(inode_t* inode, void* buf, size_t count, off_t offset) {
    return vfs_read(inode, buf, count, offset);
}

// 验证ELF文件头
static int elf_validate_header(elf32_ehdr_t* ehdr) {
    // 检查ELF魔数
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        kprintf("[ELF] Invalid ELF magic number\n");
        return -1;
    }
    
    // 检查ELF类（必须是32位）
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
        kprintf("[ELF] Only 32-bit ELF files are supported\n");
        return -1;
    }
    
    // 检查数据编码（必须是小端）
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        kprintf("[ELF] Only little-endian ELF files are supported\n");
        return -1;
    }
    
    // 检查文件类型（必须是可执行文件）
    if (ehdr->e_type != ET_EXEC) {
        kprintf("[ELF] Only executable ELF files are supported\n");
        return -1;
    }
    
    return 0;
}

// 加载ELF段到内存
static int elf_load_segment(elf32_phdr_t* phdr, inode_t* inode) {
    // 检查段类型（只处理可加载段）
    if (phdr->p_type != PT_LOAD) {
        return 0;
    }
    
    // 计算需要分配的内存大小
    size_t mem_size = phdr->p_memsz;
    if (mem_size == 0) {
        return 0;
    }
    
    // 分配内存（使用kmalloc）
    void* mem = kmalloc(mem_size);
    if (!mem) {
        kprintf("[ELF] Failed to allocate memory for segment\n");
        return -1;
    }
    
    // 读取段内容
    if (phdr->p_filesz > 0) {
        ssize_t bytes_read = read_file(inode, mem, phdr->p_filesz, phdr->p_offset);
        if (bytes_read != phdr->p_filesz) {
            kprintf("[ELF] Failed to read segment data\n");
            kfree(mem);
            return -1;
        }
    }
    
    // 初始化未初始化的数据（.bss段）
    if (phdr->p_filesz < phdr->p_memsz) {
        memset(mem + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
    }
    
    // 映射内存到进程的虚拟地址空间
    // 简化实现：直接使用物理地址作为虚拟地址（实际系统中需要使用页表映射）
    // (page_directory_t 是 uint32_t 别名，此处跳过页表映射)
    (void)mem;
    (void)phdr;
    
    // Debug: Loaded segment at 0x%x-0x%x (flags: 0x%x)
    // kprintf("[ELF] Loaded segment at 0x%x-0x%x (flags: 0x%x)\n", 
    //         phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz, phdr->p_flags);
    
    return 0;
}

// 加载ELF文件
int elf_load(const char* path, uint32_t* entry_point) {
    inode_t* inode = NULL;
    elf32_ehdr_t ehdr;
    ssize_t bytes_read;
    int ret = -1;
    
    /* 嵌入式二进制：直接从内存加载，绕过 VFS */
    if (strcmp(path, "/bin/icm_shell") == 0) {
        const uint8_t *base = ICM_SHELL_DATA;
        unsigned int   blen = ICM_SHELL_SIZE;
        elf32_ehdr_t ehdr2;
        serial_write_string("[ELF] mem_read header done\n");
        mem_read(base, blen, &ehdr2, sizeof(ehdr2), 0);
        serial_write_string("[ELF] validating header\n");
        if (elf_validate_header(&ehdr2) < 0) return -1;
        elf32_phdr_t phdr2;
        serial_write_string("[ELF] header valid, loading segments\n");
        for (int ii = 0; ii < ehdr2.e_phnum; ii++) {
            mem_read(base, blen, &phdr2, sizeof(phdr2),
                     ehdr2.e_phoff + ii * ehdr2.e_phentsize);
        serial_write_string("[ELF] checking segment type\n");
            if (phdr2.p_type != 1) continue; /* PT_LOAD */
        serial_write_string("[ELF] mapping pages\n");
            /* 映射用户态页 */
            uint32_t vaddr = phdr2.p_vaddr & ~0xFFF;
            uint32_t vend  = (phdr2.p_vaddr + phdr2.p_memsz + 0xFFF) & ~0xFFF;
            while (vaddr < vend) {
                uint32_t frame = alloc_frame();
                if (frame != 0xFFFFFFFFU) map_page((void*)(uintptr_t)vaddr, frame * 0x1000, 0x7);
                vaddr += 0x1000;
            }
        serial_write_string("[ELF] copying segment data\n");
            uint8_t *dst = (uint8_t *)(uintptr_t)phdr2.p_vaddr;
            mem_read(base, blen, dst, phdr2.p_filesz, phdr2.p_offset);
            if (phdr2.p_memsz > phdr2.p_filesz)
                memset(dst + phdr2.p_filesz, 0, phdr2.p_memsz - phdr2.p_filesz);
        }
        serial_write_string("[ELF] all segments loaded\n");
        *entry_point = ehdr2.e_entry;
        kprintf("[ELF] loaded /bin/icm_shell from embed, entry=0x%x\n", ehdr2.e_entry);
        return 0;
    }

    // 打开文件
    if (vfs_open(path, 0, &inode) < 0) {
        kprintf("[ELF] Failed to open file %s\n", path);
        return -1;
    }
    
    // 读取ELF文件头
    bytes_read = read_file(inode, &ehdr, sizeof(ehdr), 0);
    if (bytes_read != sizeof(ehdr)) {
        kprintf("[ELF] Failed to read ELF header\n");
        goto cleanup;
    }
    
    // 验证ELF文件头
    if (elf_validate_header(&ehdr) < 0) {
        goto cleanup;
    }
    
    // 读取程序头表
    elf32_phdr_t phdr;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        // 计算程序头偏移量
        off_t phdr_offset = ehdr.e_phoff + i * ehdr.e_phentsize;
        
        // 读取程序头
        bytes_read = read_file(inode, &phdr, sizeof(phdr), phdr_offset);
        if (bytes_read != sizeof(phdr)) {
            kprintf("[ELF] Failed to read program header %d\n", i);
            goto cleanup;
        }
        
        // 加载段
        if (elf_load_segment(&phdr, inode) < 0) {
            goto cleanup;
        }
    }
    
    // 设置程序入口点
    *entry_point = ehdr.e_entry;
    
    // Debug: Loaded ELF file
    // kprintf("[ELF] Loaded ELF file %s, entry point: 0x%x\n", path, ehdr.e_entry);
    
    ret = 0;
    
cleanup:
    vfs_close(inode);
    return ret;
}

// 清理ELF加载器资源
void elf_cleanup(void) {
    // 简化实现：不需要特殊清理
}
