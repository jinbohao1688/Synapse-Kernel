#include <mm/paging.h>
#include <string.h>
#include <vga.h>
#include <serial.h>

// 物理帧分配器状态
static frame_allocator_t frame_allocator = {0};

// 全局页目录指针
static page_directory_t* kernel_page_dir = NULL;

// 初始化物理帧分配器
static void init_frame_allocator(void)
{
    // 假设我们有 128MB 物理内存
    uint32_t total_memory = 128 * 1024 * 1024;
    frame_allocator.total_frames = total_memory / PAGE_SIZE;
    frame_allocator.used_frames = 0;
    
    // 分配帧位图（每个帧占用 1 位）
    uint32_t bitmap_size = (frame_allocator.total_frames + 31) / 32;
    frame_allocator.frame_bitmap = (uint32_t*)0x100000; // 使用 1MB 处的内存
    
    // 初始化位图为 0（所有帧可用）
    memset(frame_allocator.frame_bitmap, 0, bitmap_size * sizeof(uint32_t));
    
    // 标记前 1MB 内存为已使用（包含内核代码和数据）
    uint32_t reserved_frames = 0x100000 / PAGE_SIZE;
    for (uint32_t i = 0; i < reserved_frames; i++) {
        frame_allocator.frame_bitmap[i / 32] |= (1 << (i % 32));
    }
    frame_allocator.used_frames = reserved_frames;
    
    kprintf("[PAGING] Frame allocator initialized: %d total frames, %d used, %d free\n", 
            frame_allocator.total_frames, frame_allocator.used_frames, 
            frame_allocator.total_frames - frame_allocator.used_frames);
}

// 分配一个物理帧
uint32_t alloc_frame(void)
{
    // 遍历帧位图，寻找第一个可用帧
    for (uint32_t i = 0; i < frame_allocator.total_frames; i++) {
        uint32_t bitmap_idx = i / 32;
        uint32_t bit_idx = i % 32;
        
        if (!(frame_allocator.frame_bitmap[bitmap_idx] & (1 << bit_idx))) {
            // 标记帧为已使用
            frame_allocator.frame_bitmap[bitmap_idx] |= (1 << bit_idx);
            frame_allocator.used_frames++;
            return i;
        }
    }
    
    // 没有可用帧
    kprintf("[ERROR] No free frames available!\n");
    return 0;
}

// 释放一个物理帧
void free_frame(uint32_t frame)
{
    if (frame >= frame_allocator.total_frames) {
        kprintf("[ERROR] Invalid frame number: %d\n", frame);
        return;
    }
    
    // 标记帧为可用
    uint32_t bitmap_idx = frame / 32;
    uint32_t bit_idx = frame % 32;
    
    if (!(frame_allocator.frame_bitmap[bitmap_idx] & (1 << bit_idx))) {
        kprintf("[ERROR] Frame %d is not allocated!\n", frame);
        return;
    }
    
    frame_allocator.frame_bitmap[bitmap_idx] &= ~(1 << bit_idx);
    frame_allocator.used_frames--;
}

// 初始化页表
static page_table_t* create_page_table(void)
{
    // 分配一个物理帧用于页表
    uint32_t frame = alloc_frame();
    if (frame == 0) {
        kprintf("[ERROR] Failed to allocate frame for page table!\n");
        return NULL;
    }
    
    // 获取页表的物理地址
    uint32_t phys_addr = frame * PAGE_SIZE;
    
    // 将页表清零
    memset((void*)phys_addr, 0, PAGE_SIZE);
    
    // 返回页表的虚拟地址
    return (page_table_t*)phys_addr;
}

// 初始化分页机制
void init_paging(void)
{
    // 初始化物理帧分配器
    init_frame_allocator();
    
    // 分配页目录
    kernel_page_dir = (page_directory_t*)alloc_frame() * PAGE_SIZE;
    memset(kernel_page_dir, 0, PAGE_SIZE);
    
    // 映射前 4MB 物理内存到相同的虚拟地址
    for (uint32_t i = 0; i < 1024; i++) {
        // 创建页表
        page_table_t* page_table = create_page_table();
        if (!page_table) {
            kprintf("[ERROR] Failed to create page table!\n");
            return;
        }
        
        // 将页表的物理地址添加到页目录
        uint32_t table_phys = ((uint32_t)page_table) & 0xFFFFF000;
        kernel_page_dir[i] = table_phys | PAGE_PRESENT | PAGE_WRITABLE;
        
        // 映射页表中的所有页
        for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            uint32_t phys_addr = (i * PAGE_TABLE_ENTRIES + j) * PAGE_SIZE;
            page_table[j] = phys_addr | PAGE_PRESENT | PAGE_WRITABLE;
        }
    }
    
    // 启用分页
    uint32_t cr0;
    asm volatile("mov %%cr0, %%eax" : "=a" (cr0));
    cr0 |= 0x80000000; // 设置 CR0.PG 位
    asm volatile("mov %%eax, %%cr0" : : "a" (cr0));
    
    kprintf("[PAGING] Paging enabled with %dMB memory mapped\n", 4);
}

// 映射一个虚拟地址到物理地址
void map_page(void* virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t addr = (uint32_t)virtual_addr;
    uint32_t dir_idx = addr >> 22; // 高 10 位
    uint32_t table_idx = (addr >> 12) & 0x3FF; // 中间 10 位
    
    // 检查页目录项是否存在
    if (!(kernel_page_dir[dir_idx] & PAGE_PRESENT)) {
        // 创建新的页表
        page_table_t* page_table = create_page_table();
        if (!page_table) {
            kprintf("[ERROR] Failed to create page table for mapping!\n");
            return;
        }
        
        // 将页表的物理地址添加到页目录
        uint32_t table_phys = ((uint32_t)page_table) & 0xFFFFF000;
        kernel_page_dir[dir_idx] = table_phys | PAGE_PRESENT | flags;
    }
    
    // 获取页表的物理地址并转换为虚拟地址
    uint32_t table_phys = kernel_page_dir[dir_idx] & 0xFFFFF000;
    page_table_t* page_table = (page_table_t*)table_phys;
    
    // 映射页
    page_table[table_idx] = (physical_addr & 0xFFFFF000) | flags | PAGE_PRESENT;
    
    // 刷新 TLB
    asm volatile("invlpg (%0)" : : "r" (virtual_addr));
}

// 取消映射一个虚拟地址
void unmap_page(void* virtual_addr)
{
    uint32_t addr = (uint32_t)virtual_addr;
    uint32_t dir_idx = addr >> 22;
    uint32_t table_idx = (addr >> 12) & 0x3FF;
    
    if (!(kernel_page_dir[dir_idx] & PAGE_PRESENT)) {
        return; // 页目录项不存在
    }
    
    // 获取页表
    uint32_t table_phys = kernel_page_dir[dir_idx] & 0xFFFFF000;
    page_table_t* page_table = (page_table_t*)table_phys;
    
    // 检查页表项是否存在
    if (page_table[table_idx] & PAGE_PRESENT) {
        // 释放物理帧
        uint32_t frame = (page_table[table_idx] & 0xFFFFF000) / PAGE_SIZE;
        free_frame(frame);
        
        // 取消映射
        page_table[table_idx] = 0;
        
        // 刷新 TLB
        asm volatile("invlpg (%0)" : : "r" (virtual_addr));
    }
}

// 获取虚拟地址对应的物理地址
uint32_t get_physical_addr(void* virtual_addr)
{
    uint32_t addr = (uint32_t)virtual_addr;
    uint32_t dir_idx = addr >> 22;
    uint32_t table_idx = (addr >> 12) & 0x3FF;
    
    if (!(kernel_page_dir[dir_idx] & PAGE_PRESENT)) {
        return 0; // 页目录项不存在
    }
    
    // 获取页表
    uint32_t table_phys = kernel_page_dir[dir_idx] & 0xFFFFF000;
    page_table_t* page_table = (page_table_t*)table_phys;
    
    if (!(page_table[table_idx] & PAGE_PRESENT)) {
        return 0; // 页表项不存在
    }
    
    // 返回物理地址
    uint32_t phys_page = page_table[table_idx] & 0xFFFFF000;
    uint32_t offset = addr & 0x00000FFF;
    
    return phys_page + offset;
}

// 获取总物理内存大小
size_t get_total_memory(void)
{
    return frame_allocator.total_frames * PAGE_SIZE;
}

// 获取已使用的物理内存大小
size_t get_used_memory(void)
{
    return frame_allocator.used_frames * PAGE_SIZE;
}

// 获取可用的物理内存大小
size_t get_free_memory(void)
{
    return (frame_allocator.total_frames - frame_allocator.used_frames) * PAGE_SIZE;
}

// 获取内核页目录
page_directory_t* get_kernel_page_dir(void)
{
    return kernel_page_dir;
}
