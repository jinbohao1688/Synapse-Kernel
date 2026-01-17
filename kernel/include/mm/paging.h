#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

// 页大小：4KB
#define PAGE_SIZE 4096

// 页目录和页表项的数量
#define PAGE_DIR_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024

// 页目录项（PDE）和页表项（PTE）的标志位
#define PAGE_PRESENT 0x1      // 页存在于物理内存中
#define PAGE_WRITABLE 0x2     // 页可写
#define PAGE_USER 0x4         // 用户级页
#define PAGE_WRITE_THROUGH 0x8 // 写透缓存
#define PAGE_CACHE_DISABLED 0x10 // 禁用缓存
#define PAGE_ACCESSED 0x20    // 页被访问过
#define PAGE_DIRTY 0x40       // 页被修改过
#define PAGE_PAT 0x80         // 页属性表
#define PAGE_GLOBAL 0x100     // 全局页（CPU 不刷新 TLB）

// 页目录项和页表项的结构（32位）
typedef uint32_t page_entry_t;

// 页目录和页表的结构
typedef page_entry_t page_directory_t[PAGE_DIR_ENTRIES];
typedef page_entry_t page_table_t[PAGE_TABLE_ENTRIES];

// 物理帧分配器状态
typedef struct {
    uint32_t total_frames;     // 总物理帧数
    uint32_t used_frames;      // 已使用的物理帧数
    uint32_t* frame_bitmap;    // 物理帧位图
} frame_allocator_t;

// 分页初始化函数
void init_paging(void);

// 物理帧分配和释放函数
uint32_t alloc_frame(void);
void free_frame(uint32_t frame);

// 页表操作函数
void map_page(void* virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(void* virtual_addr);
uint32_t get_physical_addr(void* virtual_addr);

// 内核堆管理函数
void* kmalloc(size_t size);
void kfree(void* ptr);
void init_kheap(void);
void get_kheap_info(size_t* total, size_t* used, size_t* free);

// 内存信息获取函数
size_t get_total_memory(void);
size_t get_used_memory(void);
size_t get_free_memory(void);

// 获取内核页目录
page_directory_t* get_kernel_page_dir(void);

#endif // PAGING_H
