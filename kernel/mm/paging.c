// paging.c — Simplified paging initialization
// All structures at fixed physical addresses; paging OFF state is sufficient.
#include <mm/paging.h>
#include <serial.h>
#include <string.h>

// Fixed physical addresses for paging structures (must match linker.ld)
#define KERNEL_PD_PHYS   0x110000U
#define KERNEL_PT0_PHYS  0x111000U   // maps 0–4 MB
#define KERNEL_PT1_PHYS  0x112000U   // maps 4–8 MB
#define BITMAP_PHYS       0x113000U   // frame bitmap
#define RESERVED_BYTES    0x800000U   // 8 MB — all reserved in bitmap

// Fixed pointers (phys addresses cast directly, valid because paging is OFF)
static uint32_t* const kernel_pd   = (uint32_t*)KERNEL_PD_PHYS;
static uint32_t* const kernel_pt0 = (uint32_t*)KERNEL_PT0_PHYS;
static uint32_t* const kernel_pt1  = (uint32_t*)KERNEL_PT1_PHYS;

// Frame allocator state
static uint32_t  fa_total_frames = 0;
static uint32_t* const frame_bitmap = (uint32_t*)BITMAP_PHYS;

void init_frame_allocator(uint32_t mem_bytes)
{
    serial_write_string("[FA] init_frame_allocator\n");
    fa_total_frames = mem_bytes / PAGE_SIZE;
    serial_write_string("[FA] total_frames=");
    serial_write_hex(fa_total_frames);
    serial_write_string("\n");

    // 1. Zero the bitmap
    uint32_t bitmap_words = (fa_total_frames + 31) / 32;
    for (uint32_t i = 0; i < bitmap_words; i++)
        frame_bitmap[i] = 0;
    serial_write_string("[FA] bitmap zeroed\n");

    // 2. Mark 0–8 MB (frames 0–2047) as reserved.
    //    This covers: kernel code/data, PD, PT0, PT1, and the bitmap itself.
    uint32_t reserved = RESERVED_BYTES / PAGE_SIZE;  // = 2048
    for (uint32_t i = 0; i < reserved; i++)
        frame_bitmap[i / 32] |= (1U << (i % 32));
    serial_write_string("[FA] reserved=");
    serial_write_hex(reserved);
    serial_write_string(" frames (0–8MB)\n");

    // 3. First real allocation will come from frame 2048+ (8MB+)
    serial_write_string("[FA] done\n");
}

uint32_t alloc_frame(void)
{
    for (uint32_t w = 0; w < (fa_total_frames + 31) / 32; w++) {
        if (frame_bitmap[w] == 0xFFFFFFFFU) continue;
        for (uint32_t b = 0; b < 32; b++) {
            if (!(frame_bitmap[w] & (1U << b))) {
                frame_bitmap[w] |= (1U << b);
                return w * 32 + b;
            }
        }
    }
    serial_write_string("[FA] FATAL: out of memory\n");
    return 0xFFFFFFFFU;
}

void free_frame(uint32_t frame)
{
    if (frame < fa_total_frames)
        frame_bitmap[frame / 32] &= ~(1U << (frame % 32));
}

// Self-reference: PDE[1023] maps to the page directory itself at virt 0xFFFFF000
#define SELF_REF_PDE 1023

// Kernel heap: PDE[768] → 0xC0000000 (starts at 4GB boundary)
#define KHEAP_VIRT_START   0xC0000000u
#define KHEAP_PDE_IDX      (KHEAP_VIRT_START >> 22)   /* = 768 */
#define KHEAP_INIT_PAGES   1024u                        /* 4 MB initial heap */

void init_paging(void)
{
    // Initialize frame allocator first
    init_frame_allocator(128 * 1024 * 1024);

    serial_write_string("[PG] step1: zero PD/PT\n");
    for (int i = 0; i < 1024; i++) kernel_pd[i]   = 0;
    for (int i = 0; i < 1024; i++) kernel_pt0[i] = 0;
    for (int i = 0; i < 1024; i++) kernel_pt1[i] = 0;
    serial_write_string("[PG] step1 done\n");

    // 2. Identity-map 0–4 MB (PT0, 1024 pages)
    for (int i = 0; i < 1024; i++)
        kernel_pt0[i] = (uint32_t)(i * PAGE_SIZE) | 0x3;  // P + RW

    //    VGA text buffer is at physical 0xB8000..0xBFFFF (32KB = 8 pages).
    //    Remap PT0[296..303] to physical 0xB8000..0xBFFFF so serial I/O works.
    for (int i = 0; i < 8; i++)
        kernel_pt0[0xB8000 / PAGE_SIZE + i] = (0xB8000 + i * PAGE_SIZE) | 0x3;

    // 3. Identity-map 4–8 MB (PT1, 1024 pages)
    for (int i = 0; i < 1024; i++)
        kernel_pt1[i] = (uint32_t)(0x400000 + i * PAGE_SIZE) | 0x3;

    // 4. Map PD into the high half (PDE[1023] → PD itself, virt 0xFFFFF000)
    serial_write_string("[PG] step4: self-ref\n");
    kernel_pd[SELF_REF_PDE] = KERNEL_PD_PHYS | 0x3;

    // 5. Attach PTs to PD
    serial_write_string("[PG] step5: attach PTs\n");
    kernel_pd[0] = KERNEL_PT0_PHYS | 0x3;
    kernel_pd[1] = KERNEL_PT1_PHYS | 0x3;

    // 6. Set up kernel heap: PDE[768] → 0xC0000000
    serial_write_string("[PG] step6: heap PT alloc\n");
    {
        uint32_t heap_pt_frame = alloc_frame();
        serial_write_string("[PG] heap_pt_frame=");
        serial_write_hex(heap_pt_frame);
        serial_write_string("\n");
        uint32_t heap_pt_phys = heap_pt_frame * PAGE_SIZE;
        serial_write_string("[PG] heap_pt_phys=");
        serial_write_hex(heap_pt_phys);
        serial_write_string("\n");
        uint32_t *heap_pt = (uint32_t *)heap_pt_phys;
        serial_write_string("[PG] before zero loop\n");
        for (int i = 0; i < 1024; i++)
            heap_pt[i] = 0;
        serial_write_string("[PG] after zero loop\n");
        for (uint32_t i = 0; i < KHEAP_INIT_PAGES; i++) {
            uint32_t f = alloc_frame();
            if (f == 0xFFFFFFFFu) { serial_write_string("[PG] OOM\n"); break; }
            heap_pt[i] = (f * PAGE_SIZE) | 0x3;
        }
        serial_write_string("[PG] after frame alloc loop\n");
        kernel_pd[KHEAP_PDE_IDX] = heap_pt_phys | 0x3;
        serial_write_string("[PG] PDE[768] set\n");
    }

    serial_write_string("[PG] step6: heap PT done, enabling paging...\n");
    // 7. Load CR3 and enable paging.
    //    All critical data (kernel code, stack, PD/PT/bitmaps) is identity-
    //    mapped in 0–8 MB, so this is safe. No serial calls after this point.
    __asm__ volatile (
        "mov %0,    %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or  $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        :
        : "r" ((uint32_t)KERNEL_PD_PHYS)
        : "eax", "memory"
    );
}

uint32_t get_total_memory(void) { return fa_total_frames * PAGE_SIZE; }
uint32_t get_used_memory(void)
{
    uint32_t used = 0;
    for (uint32_t w = 0; w < (fa_total_frames + 31) / 32; w++)
        for (uint32_t b = 0; b < 32; b++)
            if (frame_bitmap[w] & (1U << b)) used++;
    return used * PAGE_SIZE;
}
uint32_t get_free_memory(void) { return get_total_memory() - get_used_memory(); }

page_directory_t* get_kernel_page_dir(void) { return (page_directory_t*)kernel_pd; }

static page_directory_t* get_current_page_dir(void)
{
    page_directory_t* pd;
    __asm__ volatile ("movl %%cr3, %0" : "=r"(pd));
    return pd;
}

void map_page(void* virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t vaddr = (uint32_t)(uintptr_t)virtual_addr;
    uint32_t pde_idx = vaddr >> 22;
    uint32_t pte_idx = (vaddr >> 12) & 0x3FF;

    /* 如果 PDE 不存在，分配一个新的 PT */
    if (!(kernel_pd[pde_idx] & 0x1)) {
        uint32_t pt_frame = alloc_frame();
        if (pt_frame == 0xFFFFFFFFU) {
            serial_write_string("[MAP] alloc PT failed\n");
            return;
        }
        uint32_t pt_phys = pt_frame * PAGE_SIZE;
        uint32_t* new_pt = (uint32_t*)(uintptr_t)pt_phys;
        for (int k = 0; k < 1024; k++) new_pt[k] = 0;
        kernel_pd[pde_idx] = pt_phys | 0x7; /* U|W|P */
    }

    /* 取 PT 物理地址，直接写 PTE */
    uint32_t pt_phys = kernel_pd[pde_idx] & ~0xFFF;
    uint32_t* pt = (uint32_t*)(uintptr_t)pt_phys;
    pt[pte_idx] = (physical_addr & ~0xFFF) | (flags & 0x7);

    /* TLB flush */
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void unmap_page(void* virtual_addr)
{
    (void)virtual_addr;
    serial_write_string("[UNMAP] stub called\n");
}

uint32_t get_physical_addr(void* virtual_addr)
{
    (void)virtual_addr;
    serial_write_string("[GPA] stub called\n");
    return 0;
}

// Alias for code that uses the old name
uint32_t get_free_mem_size(void) { return get_free_memory(); }
