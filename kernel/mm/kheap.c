#include <mm/paging.h>
#include <string.h>
#include <vga.h>

// 内核堆配置
#define KHEAP_START 0xC0000000
#define KHEAP_SIZE 0x1000000  // 16MB

// 堆块头结构
typedef struct heap_block {
    size_t size;              // 块大小（包括头）
    struct heap_block* next;  // 下一个块
    struct heap_block* prev;  // 上一个块
    bool is_free;             // 是否空闲
} heap_block_t;

// 内核堆状态
typedef struct {
    heap_block_t* free_list;  // 空闲块链表
    void* start;              // 堆起始地址
    void* end;                // 堆结束地址
    size_t total_size;        // 总大小
    size_t used_size;         // 已使用大小
} kheap_t;

static kheap_t kheap = {0};

// 初始化内核堆
void init_kheap(void)
{
    // 初始化堆
    kheap.start = (void*)KHEAP_START;
    kheap.end = (void*)(KHEAP_START + KHEAP_SIZE);
    kheap.total_size = KHEAP_SIZE;
    kheap.used_size = 0;
    
    // 创建初始空闲块
    kheap.free_list = (heap_block_t*)kheap.start;
    kheap.free_list->size = KHEAP_SIZE;
    kheap.free_list->next = NULL;
    kheap.free_list->prev = NULL;
    kheap.free_list->is_free = true;
    
    kprintf("[KHEAP] Kernel heap initialized at 0x%x-0x%x (%dMB)\n", 
            KHEAP_START, KHEAP_START + KHEAP_SIZE, KHEAP_SIZE / (1024 * 1024));
}

// 对齐大小到最近的 8 字节
static size_t align_size(size_t size)
{
    return (size + 7) & ~7;
}

// 寻找适合大小的空闲块
static heap_block_t* find_free_block(size_t size)
{
    heap_block_t* current = kheap.free_list;
    
    // 首次适配算法
    while (current) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// 分割块
static void split_block(heap_block_t* block, size_t size)
{
    if (block->size - size < sizeof(heap_block_t) + 8) {
        // 块太小，无法分割
        return;
    }
    
    heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + size);
    new_block->size = block->size - size;
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    
    block->next = new_block;
    block->size = size;
}

// 合并相邻的空闲块
static void merge_blocks(heap_block_t* block)
{
    // 合并下一个块
    if (block->next && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // 合并上一个块
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;
    }
}

// 内核内存分配
void* kmalloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    // 计算实际需要的大小（包括堆块头）
    size_t actual_size = align_size(size) + sizeof(heap_block_t);
    
    // 寻找合适的空闲块
    heap_block_t* block = find_free_block(actual_size);
    if (!block) {
        kprintf("[ERROR] Out of memory!\n");
        return NULL;
    }
    
    // 分割块
    split_block(block, actual_size);
    
    // 标记块为已使用
    block->is_free = false;
    kheap.used_size += actual_size;
    
    // 返回可用内存的地址（跳过块头）
    return (void*)((uint8_t*)block + sizeof(heap_block_t));
}

// 内核内存释放
void kfree(void* ptr)
{
    if (!ptr) {
        return;
    }
    
    // 获取块头
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    
    if (block->is_free) {
        kprintf("[ERROR] Double free detected!\n");
        return;
    }
    
    // 标记块为空闲
    block->is_free = true;
    kheap.used_size -= block->size;
    
    // 合并相邻的空闲块
    merge_blocks(block);
}

// 获取内核堆信息
void get_kheap_info(size_t* total, size_t* used, size_t* free)
{
    if (total) {
        *total = kheap.total_size;
    }
    if (used) {
        *used = kheap.used_size;
    }
    if (free) {
        *free = kheap.total_size - kheap.used_size;
    }
}
