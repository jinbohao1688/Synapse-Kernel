#include <fs/ramfs.h>
#include <mm/kheap.h>
#include <string.h>
#include <vga.h>

// 全局变量
static uint32_t next_inode = 1;

// RamFS文件操作函数声明
static int ramfs_open(inode_t* inode, file_t* file);
static int ramfs_close(file_t* file);
static ssize_t ramfs_read(file_t* file, void* buf, size_t count, off_t offset);
static ssize_t ramfs_write(file_t* file, const void* buf, size_t count, off_t offset);
static int ramfs_create(inode_t* parent, const char* name, file_type_t type, inode_t** result);
static int ramfs_mkdir(inode_t* parent, const char* name);
static int ramfs_rmdir(inode_t* parent, const char* name);
static int ramfs_unlink(inode_t* parent, const char* name);
static int ramfs_readdir(inode_t* inode, void* buf, size_t count, off_t offset);

// RamFS文件操作结构体
file_operations_t ramfs_file_ops = {
    .open = ramfs_open,
    .close = ramfs_close,
    .read = ramfs_read,
    .write = ramfs_write,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
    .rmdir = ramfs_rmdir,
    .unlink = ramfs_unlink,
    .readdir = ramfs_readdir
};

// 初始化RamFS
int ramfs_init(void) {
    kprintf("[RAMFS] RamFS initialized\n");
    return 0;
}

// 创建新的RamFS inode
static ramfs_inode_t* ramfs_alloc_inode(file_type_t type) {
    ramfs_inode_t* ramfs_inode = (ramfs_inode_t*)kmalloc(sizeof(ramfs_inode_t));
    if (!ramfs_inode) {
        return NULL;
    }
    
    // 初始化基础inode结构
    inode_t* inode = &ramfs_inode->base;
    inode->inode = next_inode++;
    inode->type = type;
    inode->size = 0;
    inode->permissions = 0755;
    inode->uid = 0;
    inode->gid = 0;
    inode->atime = 0;
    inode->mtime = 0;
    inode->ctime = 0;
    inode->private_data = ramfs_inode;
    
    // 初始化RamFS特定字段
    ramfs_inode->entries = NULL;
    ramfs_inode->data = NULL;
    ramfs_inode->data_size = 0;
    ramfs_inode->data_capacity = 0;
    
    return ramfs_inode;
}

// 创建RamFS根节点
inode_t* ramfs_create_root(void) {
    ramfs_inode_t* root_inode = ramfs_alloc_inode(FILE_TYPE_DIRECTORY);
    if (!root_inode) {
        return NULL;
    }
    
    // 创建当前目录和父目录项
    ramfs_dir_entry_t* dot_entry = (ramfs_dir_entry_t*)kmalloc(sizeof(ramfs_dir_entry_t));
    ramfs_dir_entry_t* dotdot_entry = (ramfs_dir_entry_t*)kmalloc(sizeof(ramfs_dir_entry_t));
    
    if (!dot_entry || !dotdot_entry) {
        kfree(dot_entry);
        kfree(dotdot_entry);
        kfree(root_inode);
        return NULL;
    }
    
    // 设置当前目录项（.）
    strcpy(dot_entry->name, ".");
    dot_entry->inode = &root_inode->base;
    dot_entry->next = dotdot_entry;
    dot_entry->prev = dotdot_entry;
    
    // 设置父目录项（..）
    strcpy(dotdot_entry->name, "..");
    dotdot_entry->inode = &root_inode->base; // 根目录的父目录是自己
    dotdot_entry->next = dot_entry;
    dotdot_entry->prev = dot_entry;
    
    // 将目录项添加到根节点
    root_inode->entries = dot_entry;
    
    kprintf("[RAMFS] Created root inode\n");
    return &root_inode->base;
}

// 在目录中查找文件
static ramfs_dir_entry_t* ramfs_find_entry(ramfs_inode_t* dir_inode, const char* name) {
    ramfs_dir_entry_t* entry = dir_inode->entries;
    
    if (!entry) {
        return NULL;
    }
    
    do {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    } while (entry != dir_inode->entries);
    
    return NULL;
}

// 打开文件
static int ramfs_open(inode_t* inode, file_t* file) {
    file->inode = inode;
    file->offset = 0;
    file->f_ops = &ramfs_file_ops;
    file->private_data = NULL;
    
    return 0;
}

// 关闭文件
static int ramfs_close(file_t* file) {
    // 简化实现：不需要特殊处理
    return 0;
}

// 读取文件
static ssize_t ramfs_read(file_t* file, void* buf, size_t count, off_t offset) {
    inode_t* inode = file->inode;
    ramfs_inode_t* ramfs_inode = (ramfs_inode_t*)inode->private_data;
    
    // 检查是否是普通文件
    if (inode->type != FILE_TYPE_REGULAR) {
        return -1;
    }
    
    // 计算实际可读的字节数
    size_t available = (offset < ramfs_inode->data_size) ? (ramfs_inode->data_size - offset) : 0;
    size_t read_count = (count < available) ? count : available;
    
    if (read_count > 0) {
        memcpy(buf, (uint8_t*)ramfs_inode->data + offset, read_count);
    }
    
    return read_count;
}

// 写入文件
static ssize_t ramfs_write(file_t* file, const void* buf, size_t count, off_t offset) {
    inode_t* inode = file->inode;
    ramfs_inode_t* ramfs_inode = (ramfs_inode_t*)inode->private_data;
    
    // 检查是否是普通文件
    if (inode->type != FILE_TYPE_REGULAR) {
        return -1;
    }
    
    // 确保有足够的空间
    size_t new_size = offset + count;
    if (new_size > ramfs_inode->data_capacity) {
        // 计算新的容量（按4KB递增）
        size_t new_capacity = ((new_size + 4095) & ~4095);
        
        // 重新分配内存
        void* new_data = kmalloc(new_capacity);
        if (!new_data) {
            return -1;
        }
        
        // 复制现有数据
        if (ramfs_inode->data) {
            memcpy(new_data, ramfs_inode->data, ramfs_inode->data_size);
            kfree(ramfs_inode->data);
        }
        
        // 更新数据指针和容量
        ramfs_inode->data = new_data;
        ramfs_inode->data_capacity = new_capacity;
    }
    
    // 写入数据
    memcpy((uint8_t*)ramfs_inode->data + offset, buf, count);
    
    // 更新文件大小
    if (new_size > ramfs_inode->data_size) {
        ramfs_inode->data_size = new_size;
        inode->size = new_size;
    }
    
    return count;
}

// 创建文件
static int ramfs_create(inode_t* parent, const char* name, file_type_t type, inode_t** result) {
    ramfs_inode_t* parent_ramfs_inode = (ramfs_inode_t*)parent->private_data;
    
    // 检查父节点是否是目录
    if (parent->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    // 检查文件是否已存在
    if (ramfs_find_entry(parent_ramfs_inode, name)) {
        return -1;
    }
    
    // 创建新的inode
    ramfs_inode_t* new_ramfs_inode = ramfs_alloc_inode(type);
    if (!new_ramfs_inode) {
        return -1;
    }
    
    // 创建目录项
    ramfs_dir_entry_t* new_entry = (ramfs_dir_entry_t*)kmalloc(sizeof(ramfs_dir_entry_t));
    if (!new_entry) {
        kfree(new_ramfs_inode);
        return -1;
    }
    
    // 设置目录项信息
    strcpy(new_entry->name, name);
    new_entry->inode = &new_ramfs_inode->base;
    
    // 将目录项添加到父目录
    if (parent_ramfs_inode->entries) {
        // 插入到链表头部
        new_entry->next = parent_ramfs_inode->entries;
        new_entry->prev = parent_ramfs_inode->entries->prev;
        parent_ramfs_inode->entries->prev->next = new_entry;
        parent_ramfs_inode->entries->prev = new_entry;
    } else {
        // 链表为空，自己形成循环
        new_entry->next = new_entry;
        new_entry->prev = new_entry;
        parent_ramfs_inode->entries = new_entry;
    }
    
    *result = &new_ramfs_inode->base;
    return 0;
}

// 创建目录
static int ramfs_mkdir(inode_t* parent, const char* name) {
    inode_t* result;
    
    // 使用create函数创建目录类型的inode
    int ret = ramfs_create(parent, name, FILE_TYPE_DIRECTORY, &result);
    if (ret < 0) {
        return ret;
    }
    
    ramfs_inode_t* dir_inode = (ramfs_inode_t*)result->private_data;
    
    // 创建当前目录和父目录项
    ramfs_dir_entry_t* dot_entry = (ramfs_dir_entry_t*)kmalloc(sizeof(ramfs_dir_entry_t));
    ramfs_dir_entry_t* dotdot_entry = (ramfs_dir_entry_t*)kmalloc(sizeof(ramfs_dir_entry_t));
    
    if (!dot_entry || !dotdot_entry) {
        kfree(dot_entry);
        kfree(dotdot_entry);
        return -1;
    }
    
    // 设置当前目录项（.）
    strcpy(dot_entry->name, ".");
    dot_entry->inode = result;
    
    // 设置父目录项（..）
    strcpy(dotdot_entry->name, "..");
    dotdot_entry->inode = parent;
    
    // 将目录项添加到新目录
    dot_entry->next = dotdot_entry;
    dot_entry->prev = dotdot_entry;
    dotdot_entry->next = dot_entry;
    dotdot_entry->prev = dot_entry;
    
    dir_inode->entries = dot_entry;
    
    return 0;
}

// 删除目录
static int ramfs_rmdir(inode_t* parent, const char* name) {
    ramfs_inode_t* parent_ramfs_inode = (ramfs_inode_t*)parent->private_data;
    
    // 检查父节点是否是目录
    if (parent->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    // 查找目录项
    ramfs_dir_entry_t* entry = ramfs_find_entry(parent_ramfs_inode, name);
    if (!entry) {
        return -1;
    }
    
    // 检查是否是目录
    ramfs_inode_t* dir_inode = (ramfs_inode_t*)entry->inode->private_data;
    if (entry->inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    // 检查目录是否为空（只包含.和..）
    ramfs_dir_entry_t* sub_entry = dir_inode->entries;
    if (sub_entry && (sub_entry->next != sub_entry->prev || strcmp(sub_entry->name, ".") != 0)) {
        return -1; // 目录不为空
    }
    
    // 从父目录中移除目录项
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    
    if (parent_ramfs_inode->entries == entry) {
        // 移除的是第一个条目
        parent_ramfs_inode->entries = (entry->next != entry) ? entry->next : NULL;
    }
    
    // 释放目录项和inode
    kfree(entry);
    kfree(dir_inode);
    
    return 0;
}

// 删除文件
static int ramfs_unlink(inode_t* parent, const char* name) {
    ramfs_inode_t* parent_ramfs_inode = (ramfs_inode_t*)parent->private_data;
    
    // 检查父节点是否是目录
    if (parent->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    // 查找文件项
    ramfs_dir_entry_t* entry = ramfs_find_entry(parent_ramfs_inode, name);
    if (!entry) {
        return -1;
    }
    
    // 检查是否是普通文件
    if (entry->inode->type != FILE_TYPE_REGULAR) {
        return -1;
    }
    
    // 从父目录中移除文件项
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    
    if (parent_ramfs_inode->entries == entry) {
        // 移除的是第一个条目
        parent_ramfs_inode->entries = (entry->next != entry) ? entry->next : NULL;
    }
    
    // 释放文件数据
    ramfs_inode_t* file_inode = (ramfs_inode_t*)entry->inode->private_data;
    if (file_inode->data) {
        kfree(file_inode->data);
    }
    
    // 释放文件项和inode
    kfree(entry);
    kfree(file_inode);
    
    return 0;
}

// 读取目录
static int ramfs_readdir(inode_t* inode, void* buf, size_t count, off_t offset) {
    ramfs_inode_t* ramfs_inode = (ramfs_inode_t*)inode->private_data;
    
    // 检查是否是目录
    if (inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    ramfs_dir_entry_t* entry = ramfs_inode->entries;
    if (!entry) {
        return 0;
    }
    
    // 计算偏移量
    off_t current_offset = 0;
    do {
        size_t entry_size = strlen(entry->name) + 1;
        if (current_offset + entry_size > offset) {
            // 找到目标条目
            size_t copy_size = entry_size - (offset - current_offset);
            if (copy_size > count) {
                copy_size = count;
            }
            
            memcpy(buf, entry->name + (offset - current_offset), copy_size);
            return copy_size;
        }
        
        current_offset += entry_size;
        entry = entry->next;
    } while (entry != ramfs_inode->entries);
    
    return 0;
}
