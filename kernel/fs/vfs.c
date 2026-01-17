#include <fs/vfs.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>

// 全局变量
static mount_point_t* mount_points = NULL;
static inode_t* root_inode = NULL;

// 初始化虚拟文件系统
void vfs_init(void) {
    mount_points = NULL;
    root_inode = NULL;
    kprintf("[VFS] Virtual File System initialized\n");
}

// 挂载文件系统
int vfs_mount(const char* device, const char* mount_point, file_operations_t* f_ops, inode_t* root_inode) {
    // 创建挂载点结构
    mount_point_t* new_mount = (mount_point_t*)kmalloc(sizeof(mount_point_t));
    if (!new_mount) {
        return -1;
    }
    
    // 分配内存并复制路径
    new_mount->mount_point = (char*)kmalloc(strlen(mount_point) + 1);
    if (!new_mount->mount_point) {
        kfree(new_mount);
        return -1;
    }
    strcpy(new_mount->mount_point, mount_point);
    
    // 设置挂载点信息
    new_mount->root_inode = root_inode;
    new_mount->f_ops = f_ops;
    new_mount->next = mount_points;
    
    // 添加到挂载点链表
    mount_points = new_mount;
    
    // 如果是根文件系统，设置全局根节点
    if (strcmp(mount_point, "/") == 0) {
        root_inode = root_inode;
    }
    
    kprintf("[VFS] Mounted filesystem at %s\n", mount_point);
    return 0;
}

// 卸载文件系统
int vfs_umount(const char* mount_point) {
    mount_point_t* current = mount_points;
    mount_point_t* prev = NULL;
    
    // 查找挂载点
    while (current) {
        if (strcmp(current->mount_point, mount_point) == 0) {
            // 从链表中移除
            if (prev) {
                prev->next = current->next;
            } else {
                mount_points = current->next;
            }
            
            // 释放资源
            kfree(current->mount_point);
            kfree(current);
            
            kprintf("[VFS] Unmounted filesystem from %s\n", mount_point);
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
    return -1;
}

// 路径解析
int vfs_path_resolve(const char* path, inode_t** result_inode, char** basename) {
    // 简化实现：仅返回根节点
    *result_inode = root_inode;
    *basename = NULL;
    return 0;
}

// 打开文件
int vfs_open(const char* path, int flags, inode_t** result_inode) {
    inode_t* inode = NULL;
    char* basename = NULL;
    
    // 解析路径
    int ret = vfs_path_resolve(path, &inode, &basename);
    if (ret < 0) {
        return ret;
    }
    
    *result_inode = inode;
    return 0;
}

// 关闭文件
int vfs_close(inode_t* inode) {
    // 简化实现：仅返回成功
    return 0;
}

// 读取文件
ssize_t vfs_read(inode_t* inode, void* buf, size_t count, off_t offset) {
    // 简化实现：返回0表示没有数据可读
    return 0;
}

// 写入文件
ssize_t vfs_write(inode_t* inode, const void* buf, size_t count, off_t offset) {
    // 简化实现：返回写入的字节数
    return count;
}

// 创建文件
int vfs_create(const char* path, file_type_t type) {
    // 简化实现：仅返回成功
    return 0;
}

// 创建目录
int vfs_mkdir(const char* path) {
    // 简化实现：仅返回成功
    return 0;
}

// 删除目录
int vfs_rmdir(const char* path) {
    // 简化实现：仅返回成功
    return 0;
}

// 删除文件
int vfs_unlink(const char* path) {
    // 简化实现：仅返回成功
    return 0;
}

// 读取目录
int vfs_readdir(const char* path, void* buf, size_t count, off_t offset) {
    // 简化实现：仅返回成功
    return 0;
}
