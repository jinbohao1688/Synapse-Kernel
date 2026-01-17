#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 文件系统类型
#define FS_TYPE_TMPFS 0x1
#define FS_TYPE_EXT2 0x2

// 文件权限
#define S_IRUSR 0x0400  // 所有者可读
#define S_IWUSR 0x0200  // 所有者可写
#define S_IXUSR 0x0100  // 所有者可执行
#define S_IRGRP 0x0040  // 组可读
#define S_IWGRP 0x0020  // 组可写
#define S_IXGRP 0x0010  // 组可执行
#define S_IROTH 0x0004  // 其他人可读
#define S_IWOTH 0x0002  // 其他人可写
#define S_IXOTH 0x0001  // 其他人可执行

// 文件类型
#define FT_REGULAR 0x1  // 普通文件
#define FT_DIRECTORY 0x2  // 目录
#define FT_SYMLINK 0x3  // 符号链接
#define FT_SPECIAL 0x4  // 特殊文件

// 文件描述符表大小
#define MAX_FILES 64

// 目录项结构
typedef struct {
    char name[256];           // 文件名
    uint32_t inode;           // inode号
    uint8_t type;             // 文件类型
    uint32_t size;            // 文件大小
} dir_entry_t;

// 文件inode结构
typedef struct {
    uint32_t inode;           // inode号
    uint32_t size;            // 文件大小
    uint32_t blocks;          // 块数
    uint8_t type;             // 文件类型
    uint16_t permissions;     // 权限
    uint32_t owner;           // 所有者
    uint32_t group;           // 组
    uint32_t atime;           // 访问时间
    uint32_t mtime;           // 修改时间
    uint32_t ctime;           // 创建时间
    void* data;               // 文件数据（简化实现）
    dir_entry_t* children;    // 子目录项（仅目录使用）
    uint32_t child_count;     // 子目录项数量
} inode_t;

// 文件描述符结构
typedef struct {
    bool used;                // 是否被使用
    inode_t* inode;           // 关联的inode
    uint32_t offset;          // 文件偏移量
    uint32_t flags;           // 打开标志
} file_descriptor_t;

// 挂载点结构
typedef struct {
    char mount_point[256];    // 挂载点路径
    uint8_t fs_type;          // 文件系统类型
    void* fs_data;            // 文件系统私有数据
} mount_point_t;

// 文件系统操作接口
typedef struct {
    inode_t* (*create)(const char* path, uint8_t type, uint16_t permissions);
    inode_t* (*open)(const char* path);
    int (*close)(inode_t* inode);
    int (*read)(inode_t* inode, void* buf, size_t count, uint32_t offset);
    int (*write)(inode_t* inode, const void* buf, size_t count, uint32_t offset);
    int (*unlink)(const char* path);
    int (*mkdir)(const char* path, uint16_t permissions);
    int (*rmdir)(const char* path);
    int (*readdir)(const char* path, dir_entry_t* entries, size_t count);
    int (*rename)(const char* old_path, const char* new_path);
} fs_operations_t;

// 初始化文件系统
extern void init_filesystem(void);

// 挂载文件系统
extern int mount(const char* source, const char* target, uint8_t fs_type, uint32_t flags);

// 文件操作
extern int open(const char* path, int flags, ...);
extern int close(int fd);
extern ssize_t read(int fd, void* buf, size_t count);
extern ssize_t write(int fd, const void* buf, size_t count);
extern int unlink(const char* path);
extern int mkdir(const char* path, int mode);
extern int rmdir(const char* path);
extern int readdir(const char* path, dir_entry_t* entries, size_t count);
extern int rename(const char* old_path, const char* new_path);
extern int lseek(int fd, off_t offset, int whence);

// 文件系统信息
extern void list_mount_points(void);
extern void fs_stat(const char* path, inode_t* stat_buf);

#endif // FS_H
