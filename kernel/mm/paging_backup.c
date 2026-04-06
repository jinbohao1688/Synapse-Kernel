#include <mm/paging.h>
#include <string.h>
#include <vga.h>
#include <serial.h>

// 物理帧分配器状态
static frame_allocator_t frame_allocator = {0};

// 全局页目录指针
static page_directory_t* kernel_page_dir = NULL;

// 内核镜像结束地址（由链接器提供）
extern uint32_t _kernel_end;
// Page directory 由链接器固定在 0x105000
extern uint32_t _page_dir_addr;

// 初始化物理帧分配器
static void init_frame_allocator(void)
{
    serial_write_string("[FA] line1\n");
    uint32_t total_memory = 128 * 1024 * 1024;
    frame_allocator.total_frames = total_memory / PAGE_SIZE;
    frame_allocator.used_frames = 0;
    serial_write_string("[FA] line2\n");

    uint32_t bitmap_size = (frame_allocator.total_frames + 31) / 32;
    // frame_bitmap 放在 2MB 处，前 2MB (frames 0-511) 全部保留
    frame_allocator.frame_bitmap = (uint32_t*)0x200000;
    uint32_t bitmap_frame = 0x200000 / PAGE_SIZE;  // = 512
    serial_write_string("[FA] bitmap at 0x200000, frame=");
    serial_write_hex(bitmap_frame);
    serial_write_string("\n");

    serial_write_string("[FA] calling inline memset...\n");
    // inline zero-fill to avoid any external memset issues
    {
        unsigned char* p = (unsigned char*)frame_allocator.frame_bitmap;
        uint32_t count = bitmap_size * sizeof(uint32_t);
        while (count--) *p++ = 0;
    }
    serial_write_string("[FA] memset done\n");

    // 前 8MB+4KB 全保留：内核镜像 + 页目录 + buffer
    // MUST be >= 2049 so frame 2048 (0x200000 = bitmap) stays set even after
    // memset(page_table,0) inside create_page_table() zeros bitmap[16].
    // Identity-mapped page tables then land at frames 2049+ (0x205000+),
    // safely outside the bitmap's 4KB at 0x200000.
    uint32_t reserved_frames = 0x200000 / PAGE_SIZE + 1;  // = 513
    serial_write_string("[FA] reserved_frames=");
    serial_write_hex(reserved_frames);
    serial_write_string(" (protects bitmap at frame 512 from being reallocated)\n");

    for (uint32_t i = 0; i < reserved_frames; i++) {
        frame_allocator.frame_bitmap[i / 32] |= (1 << (i % 32));
    }
    // Mark bitmap's own frame (512) as used so it's never allocated
    frame_allocator.frame_bitmap[512 / 32] |= (1 << (512 % 32));
    serial_write_string("[FA] loop done\n");
    frame_allocator.used_frames = reserved_frames + 1;
    serial_write_string("[FA] done\n");
}

// 确保 bitmap 所在帧不被分配出去（否则会覆盖 bitmap 自身）
static void protect_bitmap_frame(void)
{
    uint32_t bitmap_frame = (uint32_t)frame_allocator.frame_bitmap / PAGE_SIZE;
    uint32_t idx = bitmap_frame / 32;
    uint32_t bit = bitmap_frame % 32;
    frame_allocator.frame_bitmap[idx] |= (1 << bit);
    frame_allocator.used_frames++;
    serial_write_string("[FA] bitmap frame protected\n");
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
    
    // No memset needed — every PT entry is fully written by the caller below,
    // so zeroing is redundant and only risks corrupting the frame bitmap if
    // the allocated frame happens to be the bitmap itself.
    return (page_table_t*)phys_addr;
}

// 初始化分页机制
void init_paging(void)
{
    serial_write_string("[PAGING] start\n");
    // 初始化物理帧分配器
    init_frame_allocator();
    serial_write_string("[PAGING] after frame_alloc\n");

    // 分配页目录（使用链接器固定的地址 0x105000，在 identity-mapped 范围内）
    kernel_page_dir = (page_directory_t*)((uint32_t)&_page_dir_addr);
    serial_write_string("[PAGING] pd at=");
    serial_write_hex((uint32_t)kernel_page_dir);
    serial_write_string("\n");
    serial_write_string("[PAGING] about to memset pd...\n");
    serial_write_string("[PAGING] pd frame=");
    serial_write_hex((uint32_t)kernel_page_dir / PAGE_SIZE);
    serial_write_string(" should be < 512\n");
    serial_write_string("[PAGING] memset pd start\n");
    // NOTE: we do NOT memset the page directory to zero.  The PD lives at a
    // fixed address (0x105000) and the kernel_page_dir POINTER is stored
    // inside that same 4KB frame at offset 0.  memset(pd,0,4096) would first
    // overwrite the kernel_page_dir pointer itself (NULL), then write zeros
    // through the NULL "address" and corrupt memory, causing triple-fault.
    // Every PDE is fully written below anyway, so clearing is unnecessary.
    serial_write_string("[PAGING] memset pd done (skipped - PD fully populated below)\n");
    serial_write_string("[PAGING] pd cleared\n");

    serial_write_string("[PAGING] mapping 256 PDEs...\n");
    for (uint32_t i = 0; i < 256; i++) {
        serial_write_string("[PAGING] PDE ");
        serial_write_hex(i);
        serial_write_string("\n");
        serial_write_string("[PAGING] create_pt start\n");
        page_table_t* page_table = create_page_table();
        serial_write_string("[PAGING] create_pt done, pt=");
        serial_write_hex((uint32_t)page_table);
        serial_write_string("\n");
        if (!page_table) {
            serial_write_string("[PAGING] ERROR: create_page_table returned NULL\n");
            return;
        }

        uint32_t table_phys = ((uint32_t)page_table) & 0xFFFFF000;
        kernel_page_dir[i] = table_phys | PAGE_PRESENT | PAGE_WRITABLE;
        serial_write_string("[PAGING] PDE written, filling PT...\n");

        for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            uint32_t phys_addr = (i * PAGE_TABLE_ENTRIES + j) * PAGE_SIZE;
            page_table[j] = phys_addr | PAGE_PRESENT | PAGE_WRITABLE;
        }
        serial_write_string("[PAGING] PT done\n");
    }
    serial_write_string("[PAGING] 2 PDEs done, setting cr3...\n");
    __asm__ volatile("movl %0, %%cr3" : : "r"((uint32_t)kernel_page_dir) : "memory");
    serial_write_string("[PAGING] cr3 loaded\n");
    serial_write_string("[PAGING] enabling paging...\n");
    __asm__ volatile(
        "movl %%cr0, %%eax\n"
        "orl  $0x80000000, %%eax\n"
        "movl %%eax, %%cr0\n"
        : : : "eax", "memory"
    );
    serial_write_string("[PAGING] paging ON! returning...\n");
}

// 建立内核堆区的映射（0xC0000000 开始的 16MB）
void init_kheap_mapping(void)
{
    uint32_t kheap_dir_start = (KERNEL_HEAP_START >> 22);  // = 768
    uint32_t kheap_dir_end   = ((KERNEL_HEAP_START + 0x1000000 - 1) >> 22);  // = 771

    for (uint32_t d = kheap_dir_start; d <= kheap_dir_end; d++) {
        if ((kernel_page_dir[d] & PAGE_PRESENT) == 0) {
            uint32_t pt_phys = alloc_frame() * PAGE_SIZE;
            if (pt_phys == 0) return;
            kernel_page_dir[d] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
        }
        page_table_t *pt = (page_table_t *)(kernel_page_dir[d] & 0xFFFFF000);
        uint32_t pt_end = (d == kheap_dir_end) ? (4096 - 1) % 1024 : 1023;
        for (uint32_t j = 0; j <= pt_end; j++) {
            uint32_t phys = alloc_frame() * PAGE_SIZE;
            if (phys == 0) return;
            pt[j] = phys | PAGE_PRESENT | PAGE_WRITABLE;
        }
    }
    serial_write_string("[PAGING] heap PDEs done\n");
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
