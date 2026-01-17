#include <fs.h>
#include <mm/paging.h>
#include <string.h>
#include <vga.h>
#include <serial.h>

// 全局变量
static mount_point_t mount_points[16];
static int mount_point_count = 0;
static file_descriptor_t file_descriptors[MAX_FILES];
static uint32_t next_inode = 1;

// 文件系统私有数据
typedef struct {
    inode_t* root_inode;
} tmpfs_data_t;

// 查找挂载点
static mount_point_t* find_mount_point(const char* path) {
    for (int i = 0; i < mount_point_count; i++) {
        if (strncmp(path, mount_points[i].mount_point, strlen(mount_points[i].mount_point)) == 0) {
            return &mount_points[i];
        }
    }
    return NULL;
}

// 解析路径（简化实现）
static inode_t* resolve_path(const char* path, mount_point_t** mount_point_out) {
    mount_point_t* mount_point = find_mount_point(path);
    if (!mount_point) {
        return NULL;
    }
    
    *mount_point_out = mount_point;
    tmpfs_data_t* fs_data = (tmpfs_data_t*)mount_point->fs_data;
    
    // 简化：仅处理根目录
    return fs_data->root_inode;
}

// tmpfs 创建文件
static inode_t* tmpfs_create(const char* path, uint8_t type, uint16_t permissions) {
    mount_point_t* mount_point;
    inode_t* parent_inode = resolve_path(path, &mount_point);
    if (!parent_inode || parent_inode->type != FT_DIRECTORY) {
        return NULL;
    }
    
    // 分配inode
    inode_t* inode = (inode_t*)kmalloc(sizeof(inode_t));
    if (!inode) {
        return NULL;
    }
    
    memset(inode, 0, sizeof(inode_t));
    inode->inode = next_inode++;
    inode->type = type;
    inode->permissions = permissions;
    inode->owner = 0;
    inode->group = 0;
    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;
    
    if (type == FT_DIRECTORY) {
        inode->children = (dir_entry_t*)kmalloc(4 * sizeof(dir_entry_t)); // 初始4个子目录项
        inode->child_count = 0;
    } else {
        inode->data = NULL;
        inode->size = 0;
    }
    
    return inode;
}

// tmpfs 打开文件
static inode_t* tmpfs_open(const char* path) {
    mount_point_t* mount_point;
    return resolve_path(path, &mount_point);
}

// tmpfs 关闭文件
static int tmpfs_close(inode_t* inode) {
    if (!inode) {
        return -1;
    }
    
    // 简化实现：不释放inode，因为它可能被其他进程使用
    return 0;
}

// tmpfs 读取文件
static int tmpfs_read(inode_t* inode, void* buf, size_t count, uint32_t offset) {
    if (!inode || inode->type != FT_REGULAR) {
        return -1;
    }
    
    if (offset >= inode->size) {
        return 0;
    }
    
    size_t read_size = (offset + count > inode->size) ? (inode->size - offset) : count;
    memcpy(buf, (char*)inode->data + offset, read_size);
    
    return read_size;
}

// tmpfs 写入文件
static int tmpfs_write(inode_t* inode, const void* buf, size_t count, uint32_t offset) {
    if (!inode || inode->type != FT_REGULAR) {
        return -1;
    }
    
    // 确保有足够的空间
    if (offset + count > inode->size) {
        void* new_data = kmalloc(offset + count);
        if (!new_data) {
            return -1;
        }
        
        if (inode->data) {
            memcpy(new_data, inode->data, inode->size);
            kfree(inode->data);
        }
        
        inode->data = new_data;
        inode->size = offset + count;
    }
    
    memcpy((char*)inode->data + offset, buf, count);
    inode->mtime = 0; // 更新修改时间
    
    return count;
}

// tmpfs 删除文件
static int tmpfs_unlink(const char* path) {
    mount_point_t* mount_point;
    inode_t* inode = resolve_path(path, &mount_point);
    if (!inode || inode->type == FT_DIRECTORY) {
        return -1;
    }
    
    if (inode->data) {
        kfree(inode->data);
    }
    kfree(inode);
    
    return 0;
}

// tmpfs 创建目录
static int tmpfs_mkdir(const char* path, uint16_t permissions) {
    inode_t* inode = tmpfs_create(path, FT_DIRECTORY, permissions);
    return inode ? 0 : -1;
}

// tmpfs 删除目录
static int tmpfs_rmdir(const char* path) {
    mount_point_t* mount_point;
    inode_t* inode = resolve_path(path, &mount_point);
    if (!inode || inode->type != FT_DIRECTORY || inode->child_count > 0) {
        return -1;
    }
    
    if (inode->children) {
        kfree(inode->children);
    }
    kfree(inode);
    
    return 0;
}

// tmpfs 读取目录
static int tmpfs_readdir(const char* path, dir_entry_t* entries, size_t count) {
    mount_point_t* mount_point;
    inode_t* inode = resolve_path(path, &mount_point);
    if (!inode || inode->type != FT_DIRECTORY) {
        return -1;
    }
    
    size_t read_count = (count < inode->child_count) ? count : inode->child_count;
    memcpy(entries, inode->children, read_count * sizeof(dir_entry_t));
    
    return read_count;
}

// tmpfs 重命名文件
static int tmpfs_rename(const char* old_path, const char* new_path) {
    // 简化实现：仅返回成功
    return 0;
}

// tmpfs 操作接口
static fs_operations_t tmpfs_ops = {
    .create = tmpfs_create,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .unlink = tmpfs_unlink,
    .mkdir = tmpfs_mkdir,
    .rmdir = tmpfs_rmdir,
    .readdir = tmpfs_readdir,
    .rename = tmpfs_rename
};

// 初始化文件系统
void init_filesystem(void) {
    // 初始化文件描述符表
    for (int i = 0; i < MAX_FILES; i++) {
        file_descriptors[i].used = false;
        file_descriptors[i].inode = NULL;
        file_descriptors[i].offset = 0;
        file_descriptors[i].flags = 0;
    }
    
    // 挂载根文件系统
    mount(NULL, "/", FS_TYPE_TMPFS, 0);
    
    kprintf("[FS] Filesystem initialized\n");
}

// 挂载文件系统
int mount(const char* source, const char* target, uint8_t fs_type, uint32_t flags) {
    if (mount_point_count >= 16) {
        return -1;
    }
    
    mount_point_t* mount_point = &mount_points[mount_point_count++];
    strcpy(mount_point->mount_point, target);
    mount_point->fs_type = fs_type;
    
    // 初始化不同类型的文件系统
    switch (fs_type) {
        case FS_TYPE_TMPFS:
            {
                tmpfs_data_t* fs_data = (tmpfs_data_t*)kmalloc(sizeof(tmpfs_data_t));
                if (!fs_data) {
                    return -1;
                }
                
                // 创建根目录inode
                fs_data->root_inode = tmpfs_create("/", FT_DIRECTORY, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
                mount_point->fs_data = fs_data;
                break;
            }
            
        default:
            return -1;
    }
    
    kprintf("[FS] Mounted filesystem type %d at %s\n", fs_type, target);
    return 0;
}

// 分配文件描述符
static int alloc_file_descriptor(inode_t* inode, uint32_t flags) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_descriptors[i].used) {
            file_descriptors[i].used = true;
            file_descriptors[i].inode = inode;
            file_descriptors[i].offset = 0;
            file_descriptors[i].flags = flags;
            return i;
        }
    }
    return -1;
}

// 文件操作：打开
int open(const char* path, int flags, ...) {
    mount_point_t* mount_point;
    inode_t* inode = resolve_path(path, &mount_point);
    if (!inode) {
        // 如果文件不存在且有O_CREAT标志，则创建文件
        if (flags & 0x40) {
            inode = tmpfs_create(path, FT_REGULAR, S_IRUSR | S_IWUSR);
        }
        if (!inode) {
            return -1;
        }
    }
    
    int fd = alloc_file_descriptor(inode, flags);
    if (fd == -1) {
        tmpfs_close(inode);
        return -1;
    }
    
    return fd;
}

// 文件操作：关闭
int close(int fd) {
    if (fd < 0 || fd >= MAX_FILES || !file_descriptors[fd].used) {
        return -1;
    }
    
    tmpfs_close(file_descriptors[fd].inode);
    file_descriptors[fd].used = false;
    file_descriptors[fd].inode = NULL;
    
    return 0;
}

// 文件操作：读取
ssize_t read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILES || !file_descriptors[fd].used) {
        return -1;
    }
    
    file_descriptor_t* fd_entry = &file_descriptors[fd];
    int result = tmpfs_read(fd_entry->inode, buf, count, fd_entry->offset);
    
    if (result > 0) {
        fd_entry->offset += result;
    }
    
    return result;
}

// 文件操作：写入
ssize_t write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILES || !file_descriptors[fd].used) {
        return -1;
    }
    
    file_descriptor_t* fd_entry = &file_descriptors[fd];
    int result = tmpfs_write(fd_entry->inode, buf, count, fd_entry->offset);
    
    if (result > 0) {
        fd_entry->offset += result;
    }
    
    return result;
}

// 文件操作：删除
int unlink(const char* path) {
    return tmpfs_unlink(path);
}

// 文件操作：创建目录
int mkdir(const char* path, int mode) {
    return tmpfs_mkdir(path, mode);
}

// 文件操作：删除目录
int rmdir(const char* path) {
    return tmpfs_rmdir(path);
}

// 文件操作：读取目录
int readdir(const char* path, dir_entry_t* entries, size_t count) {
    return tmpfs_readdir(path, entries, count);
}

// 文件操作：重命名
int rename(const char* old_path, const char* new_path) {
    return tmpfs_rename(old_path, new_path);
}

// 文件操作：定位
int lseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= MAX_FILES || !file_descriptors[fd].used) {
        return -1;
    }
    
    file_descriptor_t* fd_entry = &file_descriptors[fd];
    
    switch (whence) {
        case 0: // SEEK_SET
            fd_entry->offset = offset;
            break;
            
        case 1: // SEEK_CUR
            fd_entry->offset += offset;
            break;
            
        case 2: // SEEK_END
            fd_entry->offset = fd_entry->inode->size + offset;
            break;
            
        default:
            return -1;
    }
    
    return fd_entry->offset;
}

// 列出挂载点
void list_mount_points(void) {
    for (int i = 0; i < mount_point_count; i++) {
        kprintf("%s: type %d\n", mount_points[i].mount_point, mount_points[i].fs_type);
    }
}

// 获取文件状态
void fs_stat(const char* path, inode_t* stat_buf) {
    mount_point_t* mount_point;
    inode_t* inode = resolve_path(path, &mount_point);
    if (inode && stat_buf) {
        memcpy(stat_buf, inode, sizeof(inode_t));
    }
}
