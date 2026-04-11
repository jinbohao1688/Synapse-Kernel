// elf.c — x86-64 ELF64 loader
#include <loader/elf.h>
#include <fs/vfs.h>
#include <mm/kheap.h>
#include <mm/paging.h>
#include <proc/task.h>
#include <vga.h>
#include <string.h>
#include "../embedded_bins.h"

typedef long ssize_t;

// Memory image reader
static ssize_t mem_read(const uint8_t* base, size_t base_len, void* buf, size_t count, size_t offset)
{
    if (offset >= base_len) return 0;
    size_t avail = base_len - offset;
    size_t n = (count < avail) ? count : avail;
    memcpy(buf, base + offset, n);
    return (ssize_t)n;
}

static ssize_t read_file(inode_t* inode, void* buf, size_t count, off_t offset)
{
    return vfs_read(inode, buf, count, offset);
}

static int elf_validate_header(elf64_ehdr_t* ehdr)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        kprintf("[ELF] Invalid ELF magic\n");
        return -1;
    }

    serial_write_string("[ELF] class=");
    serial_write_hex((uint32_t)ehdr->e_ident[EI_CLASS]);
    serial_write_string("\n");
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        kprintf("[ELF] Only 64-bit ELF supported\n");
        return -1;
    }

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        kprintf("[ELF] Only little-endian ELF supported\n");
        return -1;
    }

    if (ehdr->e_type != ET_EXEC) {
        kprintf("[ELF] Only executable ELF supported\n");
        return -1;
    }

    return 0;
}

static int elf_load_segment(elf64_phdr_t* phdr, inode_t* inode)
{
    if (phdr->p_type != PT_LOAD) return 0;
    if (phdr->p_memsz == 0) return 0;

    // Allocate memory for segment
    void* mem = kmalloc((size_t)phdr->p_memsz);
    if (!mem) {
        kprintf("[ELF] OOM for segment\n");
        return -1;
    }

    // Read segment data
    if (phdr->p_filesz > 0) {
        ssize_t bytes_read = read_file(inode, mem, (size_t)phdr->p_filesz, (off_t)phdr->p_offset);
        if (bytes_read != (ssize_t)phdr->p_filesz) {
            kprintf("[ELF] Read segment failed\n");
            return -1;
        }
    }

    // Zero BSS
    if (phdr->p_filesz < phdr->p_memsz) {
        memset((uint8_t*)mem + phdr->p_filesz, 0,
               (size_t)(phdr->p_memsz - phdr->p_filesz));
    }

    (void)mem;
    (void)phdr;
    return 0;
}

int elf_load(const char* path, uint64_t* entry_point)
{
    inode_t* inode = NULL;
    elf64_ehdr_t ehdr;
    ssize_t bytes_read;
    int ret = -1;

    /* Embedded binary: load directly from memory, bypass VFS */
    if (strcmp(path, "/bin/icm_shell") == 0) {
        const uint8_t* base = ICM_SHELL_DATA;
        unsigned int blen = ICM_SHELL_SIZE;
        elf64_ehdr_t ehdr2;

        serial_write_string("[ELF] mem_read header done\n");
        mem_read(base, blen, &ehdr2, sizeof(ehdr2), 0);
        serial_write_string("[ELF] validating header\n");
        if (elf_validate_header(&ehdr2) < 0) return -1;

        elf64_phdr_t phdr2;
        serial_write_string("[ELF] header valid, loading segments\n");
        for (int ii = 0; ii < ehdr2.e_phnum; ii++) {
            mem_read(base, blen, &phdr2, sizeof(phdr2),
                     (size_t)(ehdr2.e_phoff + ii * ehdr2.e_phentsize));

            serial_write_string("[ELF] checking segment type\n");
            if (phdr2.p_type != 1) continue;  /* PT_LOAD */

            serial_write_string("[ELF] mapping pages\n");
            /* Map user pages */
            uint64_t vaddr = phdr2.p_vaddr & ~0xFFFULL;
            uint64_t vend  = (phdr2.p_vaddr + phdr2.p_memsz + 0xFFFULL) & ~0xFFFULL;
            while (vaddr < vend) {
                uint64_t frame = alloc_frame();
                if (frame != ~0ULL)
                    map_page(vaddr, frame * 0x1000ULL,
                             PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
                vaddr += 0x1000ULL;
            }

            serial_write_string("[ELF] copying segment data\n");
            uint8_t* dst = (uint8_t*)(uintptr_t)phdr2.p_vaddr;
            mem_read(base, blen, dst, (size_t)phdr2.p_filesz, (size_t)phdr2.p_offset);
            if (phdr2.p_memsz > phdr2.p_filesz)
                memset(dst + phdr2.p_filesz, 0,
                       (size_t)(phdr2.p_memsz - phdr2.p_filesz));
        }

        serial_write_string("[ELF] all segments loaded\n");

        /* ── Set up user stack ──────────────────────────────────── */
        uint64_t user_stack_top = 0x500000ULL;  /* 5MB virtual — user stack */
        uint64_t ustack_frame = alloc_frame();
        if (ustack_frame != ~0ULL) {
            map_page(user_stack_top - 0x1000ULL, ustack_frame * 0x1000ULL,
                    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            serial_write_string("[ELF] user stack mapped\n");
        }

        /* ── Build kernel trampoline stack for iretq ──────────────────
           iretq frame on kernel stack (low → high):
             [rsp+0]  = RIP   ←  top when pushed
             [rsp+8]  = CS
             [rsp+16] = RFLAGS
             [rsp+24] = RSP   (user stack pointer)
             [rsp+32] = SS
           iretq pops: RIP, CS, RFLAGS, RSP, SS (5 × 8 bytes = 40 bytes)
        */
        uint8_t* kstack = (uint8_t*)kmalloc(8192);
        if (!kstack) {
            serial_write_string("[ELF] FATAL: cannot alloc kernel stack\n");
            return -1;
        }
        /* 对齐到16字节，iretq压栈顺序：SS RSP RFLAGS CS RIP */
        uint64_t rsp_tramp = ((uint64_t)(uintptr_t)(kstack + 8192)) & ~0xFULL;
        rsp_tramp -= 8; *(uint64_t*)(rsp_tramp) = 0x23;           /* SS  RPL=3 */
        rsp_tramp -= 8; *(uint64_t*)(rsp_tramp) = user_stack_top; /* RSP */
        rsp_tramp -= 8; *(uint64_t*)(rsp_tramp) = 0x202;          /* RFLAGS IF=1 */
        rsp_tramp -= 8; *(uint64_t*)(rsp_tramp) = 0x1B;           /* CS  RPL=3 */
        rsp_tramp -= 8; *(uint64_t*)(rsp_tramp) = ehdr2.e_entry;  /* RIP */

        /* Return the trampoline RSP via entry_point */
        *entry_point = rsp_tramp;
        serial_write_string("[ELF] trampoline rsp=0x");
        serial_write_hex64(rsp_tramp);
        serial_write_string(" user_entry=0x");
        serial_write_hex64(ehdr2.e_entry);
        serial_write_string("\n");
        return 0;
    }

    /* Load from VFS */
    if (vfs_open(path, 0, &inode) < 0) {
        kprintf("[ELF] Failed to open file %s\n", path);
        return -1;
    }

    bytes_read = read_file(inode, &ehdr, sizeof(ehdr), 0);
    if (bytes_read != sizeof(ehdr)) {
        kprintf("[ELF] Failed to read ELF header\n");
        goto cleanup;
    }

    if (elf_validate_header(&ehdr) < 0) {
        goto cleanup;
    }

    elf64_phdr_t phdr;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        off_t phdr_offset = (off_t)(ehdr.e_phoff + (uint64_t)i * ehdr.e_phentsize);
        bytes_read = read_file(inode, &phdr, sizeof(phdr), phdr_offset);
        if (bytes_read != sizeof(phdr)) {
            kprintf("[ELF] Failed to read program header %d\n", i);
            goto cleanup;
        }
        if (elf_load_segment(&phdr, inode) < 0) {
            goto cleanup;
        }
    }

    *entry_point = ehdr.e_entry;
    ret = 0;

cleanup:
    vfs_close(inode);
    return ret;
}

void elf_cleanup(void)
{
    // Simplified: no special cleanup needed
}

/* 跳转到用户态，设置段寄存器后 iretq */
void __attribute__((noreturn)) jump_to_user(uint64_t rsp) {
    __asm__ volatile(
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %0, %%rsp\n\t"
        "iretq"
        : : "r"(rsp) : "memory"
    );
    __builtin_unreachable();
}
