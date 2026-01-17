#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>

// ELF文件类型定义

// 32位ELF文件头
typedef struct {
    uint8_t  e_ident[16];       // ELF标识
    uint16_t e_type;            // 文件类型
    uint16_t e_machine;         // 机器类型
    uint32_t e_version;         // ELF版本
    uint32_t e_entry;           // 程序入口地址
    uint32_t e_phoff;           // 程序头表偏移量
    uint32_t e_shoff;           // 节头表偏移量
    uint32_t e_flags;           // 处理器特定标志
    uint16_t e_ehsize;          // ELF文件头大小
    uint16_t e_phentsize;       // 程序头表项大小
    uint16_t e_phnum;           // 程序头表项数量
    uint16_t e_shentsize;       // 节头表项大小
    uint16_t e_shnum;           // 节头表项数量
    uint16_t e_shstrndx;        // 字符串表在节头表中的索引
} elf32_ehdr_t;

// 32位ELF程序头
typedef struct {
    uint32_t p_type;            // 段类型
    uint32_t p_offset;          // 段在文件中的偏移量
    uint32_t p_vaddr;           // 段在内存中的虚拟地址
    uint32_t p_paddr;           // 段在内存中的物理地址
    uint32_t p_filesz;          // 段在文件中的大小
    uint32_t p_memsz;           // 段在内存中的大小
    uint32_t p_flags;           // 段的访问权限标志
    uint32_t p_align;           // 段的对齐要求
} elf32_phdr_t;

// 32位ELF节头
typedef struct {
    uint32_t sh_name;           // 节名在字符串表中的索引
    uint32_t sh_type;           // 节类型
    uint32_t sh_flags;          // 节标志
    uint32_t sh_addr;           // 节在内存中的虚拟地址
    uint32_t sh_offset;         // 节在文件中的偏移量
    uint32_t sh_size;           // 节的大小
    uint32_t sh_link;           // 节头表中相关节的索引
    uint32_t sh_info;           // 节的额外信息
    uint32_t sh_addralign;      // 节的对齐要求
    uint32_t sh_entsize;        // 节中每个条目的大小
} elf32_shdr_t;

// ELF标识值
#define EI_MAG0     0           // 魔数第0字节
#define EI_MAG1     1           // 魔数第1字节
#define EI_MAG2     2           // 魔数第2字节
#define EI_MAG3     3           // 魔数第3字节
#define EI_CLASS    4           // 类标识（1=32位，2=64位）
#define EI_DATA     5           // 数据编码（1=小端，2=大端）
#define EI_VERSION  6           // ELF版本
#define EI_OSABI    7           // OS/ABI标识
#define EI_ABIVERSION 8         // ABI版本
#define EI_PAD      9           // 填充字节

// ELF魔数
#define ELFMAG0     0x7f        // ELF魔数
#define ELFMAG1     'E'         // ELF魔数
#define ELFMAG2     'L'         // ELF魔数
#define ELFMAG3     'F'         // ELF魔数

// ELF类
#define ELFCLASSNONE 0          // 无效类
#define ELFCLASS32   1          // 32位ELF
#define ELFCLASS64   2          // 64位ELF

// ELF数据编码
#define ELFDATANONE 0          // 无效数据编码
#define ELFDATA2LSB  1          // 小端
#define ELFDATA2MSB  2          // 大端

// ELF文件类型
#define ET_NONE     0           // 无效类型
#define ET_REL      1           // 可重定位文件
#define ET_EXEC     2           // 可执行文件
#define ET_DYN      3           // 动态共享对象
#define ET_CORE     4           // 核心转储文件

// 程序头类型
#define PT_NULL     0           // 无效段
#define PT_LOAD     1           // 可加载段
#define PT_DYNAMIC  2           // 动态链接信息
#define PT_INTERP   3           // 解释器路径
#define PT_NOTE     4           // 辅助信息
#define PT_SHLIB    5           // 保留
#define PT_PHDR     6           // 程序头表自身

// 程序头标志
#define PF_X        0x01        // 可执行
#define PF_W        0x02        // 可写
#define PF_R        0x04        // 可读

// ELF加载器函数声明
extern int elf_load(const char* path, uint32_t* entry_point);

extern void elf_cleanup(void);

#endif // LOADER_ELF_H
