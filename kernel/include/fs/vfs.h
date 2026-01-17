#ifndef FS_VFS_H
#define FS_VFS_H

#include <stdint.h>
#include <stddef.h>

// 文件类型枚举
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_CHAR_DEVICE,
    FILE_TYPE_BLOCK_DEVICE,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_SOCKET,
    FILE_TYPE_FIFO
} file_type_t;

// 节点结构
typedef struct inode {
    uint32_t inode;           // 节点号
    file_type_t type;         // 文件类型
    uint32_t size;            // 文件大小
    uint32_t permissions;     // 权限
    uint32_t uid;             // 用户ID
    uint32_t gid;             // 组ID
    uint32_t atime;           // 访问时间
    uint32_t mtime;           // 修改时间
    uint32_t ctime;           // 创建时间
    void* private_data;       // 文件系统特定数据
} inode_t;

// 文件操作结构体
typedef struct file_operations {
    int (*open)(struct inode* inode, struct file* file);
    int (*close)(struct file* file);
    ssize_t (*read)(struct file* file, void* buf, size_t count, off_t offset);
    ssize_t (*write)(struct file* file, const void* buf, size_t count, off_t offset);
    int (*create)(struct inode* parent, const char* name, file_type_t type, inode_t** result);
    int (*mkdir)(struct inode* parent, const char* name);
    int (*rmdir)(struct inode* parent, const char* name);
    int (*unlink)(struct inode* parent, const char* name);
    int (*readdir)(struct inode* inode, void* buf, size_t count, off_t offset);
} file_operations_t;

// 文件描述符结构
typedef struct file {
    inode_t* inode;           // 关联的inode
    int flags;                // 打开标志
    off_t offset;             // 当前偏移量
    file_operations_t* f_ops; // 文件操作函数
    void* private_data;       // 文件特定数据
} file_t;

// 挂载点结构
typedef struct mount_point {
    char* mount_point;        // 挂载点路径
    inode_t* root_inode;      // 文件系统根节点
    file_operations_t* f_ops; // 文件系统操作函数
    struct mount_point* next; // 下一个挂载点
} mount_point_t;

// 初始化虚拟文件系统
void vfs_init(void);

// VFS核心操作函数
int vfs_open(const char* path, int flags, inode_t** result_inode);
int vfs_close(inode_t* inode);
ssize_t vfs_read(inode_t* inode, void* buf, size_t count, off_t offset);
ssize_t vfs_write(inode_t* inode, const void* buf, size_t count, off_t offset);
int vfs_create(const char* path, file_type_t type);
int vfs_mkdir(const char* path);
int vfs_rmdir(const char* path);
int vfs_unlink(const char* path);
int vfs_readdir(const char* path, void* buf, size_t count, off_t offset);

// 挂载操作
int vfs_mount(const char* device, const char* mount_point, file_operations_t* f_ops, inode_t* root_inode);
int vfs_umount(const char* mount_point);

// 路径解析
int vfs_path_resolve(const char* path, inode_t** result_inode, char** basename);

#endif // FS_VFS_H
