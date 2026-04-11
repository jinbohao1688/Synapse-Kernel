#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>

// ELF file type definitions

// 64-bit ELF file header
typedef struct {
    uint8_t  e_ident[16];       // ELF identification
    uint16_t e_type;            // Object file type
    uint16_t e_machine;         // Architecture
    uint32_t e_version;         // Object file version
    uint64_t e_entry;           // Entry point virtual address
    uint64_t e_phoff;           // Program header table file offset
    uint64_t e_shoff;           // Section header table file offset
    uint32_t e_flags;           // Processor-specific flags
    uint16_t e_ehsize;          // ELF header size
    uint16_t e_phentsize;       // Program header table entry size
    uint16_t e_phnum;           // Program header table entry count
    uint16_t e_shentsize;       // Section header table entry size
    uint16_t e_shnum;           // Section header table entry count
    uint16_t e_shstrndx;        // Section header string table index
} elf64_ehdr_t;

// 64-bit ELF program header
typedef struct {
    uint32_t p_type;            // Segment type
    uint32_t p_flags;          // Segment flags
    uint64_t p_offset;          // Segment file offset
    uint64_t p_vaddr;           // Segment virtual address
    uint64_t p_paddr;           // Segment physical address
    uint64_t p_filesz;          // Segment size in file
    uint64_t p_memsz;           // Segment size in memory
    uint64_t p_align;           // Segment alignment
} elf64_phdr_t;

// 64-bit ELF section header
typedef struct {
    uint32_t sh_name;           // Section name (string tbl index)
    uint32_t sh_type;           // Section type
    uint64_t sh_flags;          // Section flags
    uint64_t sh_addr;           // Section virtual address
    uint64_t sh_offset;         // Section file offset
    uint64_t sh_size;           // Section size
    uint32_t sh_link;           // Extra link info
    uint32_t sh_info;           // Additional section info
    uint64_t sh_addralign;      // Section alignment
    uint64_t sh_entsize;        // Entry size if section holds table
} elf64_shdr_t;

// ELF identification values
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_ABIVERSION 8
#define EI_PAD      9

// ELF magic number
#define ELFMAG0     0x7f
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

// ELF class
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

// ELF data encoding
#define ELFDATANONE 0
#define ELFDATA2LSB  1
#define ELFDATA2MSB  2

// ELF file types
#define ET_NONE     0
#define ET_REL      1
#define ET_EXEC     2
#define ET_DYN      3
#define ET_CORE     4

// Program header types
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6

// Program header flags
#define PF_X        0x01
#define PF_W        0x02
#define PF_R        0x04

// ELF loader function declarations
extern int elf_load(const char* path, uint64_t* entry_point);
extern void elf_cleanup(void);

#endif // LOADER_ELF_H
