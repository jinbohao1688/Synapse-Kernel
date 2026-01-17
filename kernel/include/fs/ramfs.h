#ifndef FS_RAMFS_H
#define FS_RAMFS_H

#include <fs/vfs.h>

// RamFS目录项结构
typedef struct ramfs_dir_entry {
    char name[256];              // 文件名
    inode_t* inode;             // 关联的inode
    struct ramfs_dir_entry* next; // 下一个目录项
    struct ramfs_dir_entry* prev; // 上一个目录项
} ramfs_dir_entry_t;

// RamFS特定的inode结构
typedef struct ramfs_inode {
    inode_t base;                // 基础inode结构
    ramfs_dir_entry_t* entries;  // 目录项链表（如果是目录）
    void* data;                  // 文件数据（如果是普通文件）
    uint32_t data_size;          // 数据大小
    uint32_t data_capacity;      // 数据容量
} ramfs_inode_t;

// RamFS文件操作函数
extern file_operations_t ramfs_file_ops;

// 初始化RamFS
extern int ramfs_init(void);

// 创建RamFS根节点
extern inode_t* ramfs_create_root(void);

#endif // FS_RAMFS_H
