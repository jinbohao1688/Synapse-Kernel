// paging.c — x86-64 4-level paging initialization
// Boot.asm sets up initial 4-level tables at fixed physical addresses.
// This file provides re-initialization, frame allocation, and page mapping.
#include <mm/paging.h>
#include <serial.h>
#include <string.h>

// Physical addresses of paging structures (must match linker.ld)
#define PML4_PHYS       0x110000UL
#define PDPT_PHYS       0x111000UL
#define PD0_PHYS        0x112000UL
#define PT0_PHYS        0x113000UL
#define PT1_PHYS        0x114000UL
#define BITMAP_PHYS     0x115000UL

// Pointers to page tables (identity-mapped by boot.asm, so these are valid)
static page_entry_t* const kernel_pml4  = (page_entry_t*) PML4_PHYS;
static page_entry_t* const kernel_pdpt  = (page_entry_t*) PDPT_PHYS;
static page_entry_t* const kernel_pd0   = (page_entry_t*)   PD0_PHYS;
static page_entry_t* const kernel_pt0   = (page_entry_t*)   PT0_PHYS;
static page_entry_t* const kernel_pt1   = (page_entry_t*)   PT1_PHYS;

// Frame allocator state
static uint64_t fa_total_frames = 0;
static uint64_t* const frame_bitmap = (uint64_t*)BITMAP_PHYS;

// Bitmap word count
static uint64_t fa_bitmap_words = 0;

// How many frames are pre-reserved (kernel, page tables, bitmap)
#define RESERVED_FRAMES  (0x200000UL / PAGE_SIZE)   // 2MB = 512 frames

void init_frame_allocator(uint64_t mem_bytes)
{
    serial_write_string("[FA] init_frame_allocator (x86-64)\n");
    fa_total_frames = mem_bytes / PAGE_SIZE;
    fa_bitmap_words = (fa_total_frames + 63) / 64;

    serial_write_string("[FA] total_frames=");
    serial_write_hex64(fa_total_frames);
    serial_write_string("\n");

    // Zero bitmap
    for (uint64_t i = 0; i < fa_bitmap_words; i++)
        frame_bitmap[i] = 0;

    // Reserve low 2MB (kernel code/data + page tables + bitmap)
    for (uint64_t i = 0; i < RESERVED_FRAMES; i++)
        frame_bitmap[i / 64] |= (1ULL << (i % 64));

    serial_write_string("[FA] reserved ");
    serial_write_hex64(RESERVED_FRAMES);
    serial_write_string(" frames\n");
    serial_write_string("[FA] done\n");
}

uint64_t alloc_frame(void)
{
    for (uint64_t w = 0; w < fa_bitmap_words; w++) {
        if (frame_bitmap[w] == ~0ULL) continue;
        for (uint64_t b = 0; b < 64; b++) {
            uint64_t bit = 1ULL << b;
            if (!(frame_bitmap[w] & bit)) {
                frame_bitmap[w] |= bit;
                return w * 64 + b;
            }
        }
    }
    serial_write_string("[FA] FATAL: out of memory\n");
    return ~0ULL;
}

void free_frame(uint64_t frame)
{
    if (frame >= fa_total_frames) return;
    frame_bitmap[frame / 64] &= ~(1ULL << (frame % 64));
}

// ---- Index extractors for 4-level paging ----
static inline uint16_t pml4_idx(uint64_t vaddr) { return (vaddr >> 39) & 0x1FF; }
static inline uint16_t pdpt_idx(uint64_t vaddr) { return (vaddr >> 30) & 0x1FF; }
static inline uint16_t pd_idx (uint64_t vaddr) { return (vaddr >> 21) & 0x1FF; }
static inline uint16_t pt_idx (uint64_t vaddr) { return (vaddr >> 12) & 0x1FF; }

page_entry_t* get_kernel_pml4(void) { return kernel_pml4; }

void init_paging(void)
{
    serial_write_string("[PG] x86-64 4-level paging init\n");

    // Paging is already enabled by boot.asm.
    // Just initialize the frame allocator and set up the kernel heap mapping.
    init_frame_allocator(128UL * 1024 * 1024);

    // --- Kernel heap mapping ---
    // KHEAP_START = 0xFFFF800100000000
    // PML4[0x100] -> new_pdpt -> new_pd -> heap_pt -> frames
    uint64_t heap_start = KERNEL_HEAP_START;
    uint16_t p4i = pml4_idx(heap_start);  // 0x100
    uint16_t p3i = pdpt_idx(heap_start);  // 0x004
    uint16_t p2i = pd_idx(heap_start);    // 0x000

    serial_write_string("[PG] heap: allocating structures\n");

    // 1. new PDPT for PML4[p4i]
    uint64_t new_pdpt_frame = alloc_frame();
    page_entry_t* new_pdpt = (page_entry_t*)(new_pdpt_frame * PAGE_SIZE);
    for (int i = 0; i < 512; i++) new_pdpt[i] = 0;
    kernel_pml4[p4i] = (new_pdpt_frame * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;

    // 2. new PD for new_pdpt[p3i]
    uint64_t heap_pd_frame = alloc_frame();
    page_entry_t* heap_pd = (page_entry_t*)(heap_pd_frame * PAGE_SIZE);
    for (int i = 0; i < 512; i++) heap_pd[i] = 0;
    new_pdpt[p3i] = (heap_pd_frame * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;

    // 3. new PT for heap_pd[p2i]
    uint64_t heap_pt_frame = alloc_frame();
    page_entry_t* heap_pt = (page_entry_t*)(heap_pt_frame * PAGE_SIZE);
    for (int i = 0; i < 512; i++) heap_pt[i] = 0;
    heap_pd[p2i] = (heap_pt_frame * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;

    // 4. map 512 frames into heap PT (2MB initial heap)
    for (int i = 0; i < 512; i++) {
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[PG] OOM\n"); break; }
        heap_pt[i] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    serial_write_string("[PG] heap mapped OK\n");

        serial_write_string("[PG] paging re-init done\n");

    /* 把 boot.asm 的临时页表切换到 kernel_pml4
     * 同时保留 boot.asm 的 identity map（PML4[0]），
     * 先复制 boot 页表的 PML4[0] 到 kernel_pml4[0]           */
    uint64_t boot_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(boot_cr3));
    page_entry_t* boot_pml4 = (page_entry_t*)(boot_cr3 & ~0xFFFULL);
    kernel_pml4[0] = boot_pml4[0];   /* 保留 identity map */

    /* 切换到 kernel_pml4 */
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)PML4_PHYS) : "memory");
    serial_write_string("[PG] CR3 switched to kernel_pml4\n");
}

// Get physical address from virtual address (4-level walk)
uint64_t get_physical_addr(uint64_t vaddr)
{
    uint16_t p4 = pml4_idx(vaddr);
    uint16_t p3 = pdpt_idx(vaddr);
    uint16_t p2 = pd_idx(vaddr);
    uint16_t p1 = pt_idx(vaddr);

    if (!(kernel_pml4[p4] & PAGE_PRESENT)) return 0;
    uint64_t pdpt_base = kernel_pml4[p4] & ~0xFFFULL;
    page_entry_t* pdpt = (page_entry_t*)pdpt_base;

    if (!(pdpt[p3] & PAGE_PRESENT)) return 0;

    // Check for 1GB page
    if (pdpt[p3] & PAGE_PS) {
        return (pdpt[p3] & ~0x3FFFFFFFULL) + ((vaddr & 0x3FFFFFFF));
    }

    uint64_t pd_base = pdpt[p3] & ~0xFFFULL;
    page_entry_t* pd = (page_entry_t*)pd_base;

    if (!(pd[p2] & PAGE_PRESENT)) return 0;

    // Check for 2MB page
    if (pd[p2] & PAGE_PS) {
        return (pd[p2] & ~0x1FFFFFULL) + (vaddr & 0x1FFFFF);
    }

    uint64_t pt_base = pd[p2] & ~0xFFFULL;
    page_entry_t* pt = (page_entry_t*)pt_base;

    if (!(pt[p1] & PAGE_PRESENT)) return 0;

    return (pt[p1] & ~0xFFFULL) + (vaddr & 0xFFFULL);
}

void map_page(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    uint16_t p4 = pml4_idx(vaddr);
    uint16_t p3 = pdpt_idx(vaddr);
    uint16_t p2 = pd_idx(vaddr);
    uint16_t p1 = pt_idx(vaddr);

    // Ensure PML4 entry exists
    if (!(kernel_pml4[p4] & PAGE_PRESENT)) {
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[MAP] no PDPT frame\n"); return; }
        page_entry_t* new_pdpt = (page_entry_t*)(f * PAGE_SIZE);
        for (int i = 0; i < PDPT_ENTRIES; i++) new_pdpt[i] = 0;
        kernel_pml4[p4] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    uint64_t pdpt_base = kernel_pml4[p4] & ~0xFFFULL;
    page_entry_t* pdpt = (page_entry_t*)pdpt_base;

    // Ensure PDPT entry exists
    if (!(pdpt[p3] & PAGE_PRESENT)) {
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[MAP] no PD frame\n"); return; }
        page_entry_t* new_pd = (page_entry_t*)(f * PAGE_SIZE);
        for (int i = 0; i < PD_ENTRIES; i++) new_pd[i] = 0;
        pdpt[p3] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Check for 1GB page — split it into 2MB pages
    if (pdpt[p3] & PAGE_PS) {
        serial_write_string("[MAP] splitting 1GB page at PDPT[");
        serial_write_hex16(p3);
        serial_write_string("]\n");
        uint64_t old_base = pdpt[p3] & ~0x3FFFFFFFULL;
        // Allocate new PD and populate with 2MB entries
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[MAP] no frame for split PD\n"); return; }
        page_entry_t* new_pd = (page_entry_t*)(f * PAGE_SIZE);
        for (int i = 0; i < 512; i++)
            new_pd[i] = (old_base + (uint64_t)i * 0x200000ULL) | PAGE_PS | PAGE_PRESENT | PAGE_WRITABLE;
        pdpt[p3] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    uint64_t pd_base = pdpt[p3] & ~0xFFFULL;
    page_entry_t* pd = (page_entry_t*)pd_base;

    // Ensure PD entry exists
    if (!(pd[p2] & PAGE_PRESENT)) {
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[MAP] no PT frame\n"); return; }
        page_entry_t* new_pt = (page_entry_t*)(f * PAGE_SIZE);
        for (int i = 0; i < PT_ENTRIES; i++) new_pt[i] = 0;
        pd[p2] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Check for 2MB page — split it into 4KB pages
    if (pd[p2] & PAGE_PS) {
        serial_write_string("[MAP] splitting 2MB page at PD[");
        serial_write_hex16(p2);
        serial_write_string("]\n");
        uint64_t old_base = pd[p2] & ~0x1FFFFFULL;
        uint64_t f = alloc_frame();
        if (f == ~0ULL) { serial_write_string("[MAP] no frame for split PT\n"); return; }
        page_entry_t* new_pt = (page_entry_t*)(f * PAGE_SIZE);
        for (int i = 0; i < 512; i++)
            new_pt[i] = (old_base + (uint64_t)i * 0x1000ULL) | PAGE_PRESENT | PAGE_WRITABLE;
        pd[p2] = (f * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    uint64_t pt_base = pd[p2] & ~0xFFFULL;
    page_entry_t* pt = (page_entry_t*)pt_base;
    pt[p1] = (paddr & ~0xFFFULL) | (flags & ~0xFFFULL) | PAGE_PRESENT;

    flush_tlb(vaddr);
}

void unmap_page(uint64_t vaddr)
{
    map_page(vaddr, 0, 0);
}

void flush_tlb(uint64_t vaddr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

uint64_t get_total_memory(void) { return fa_total_frames * PAGE_SIZE; }

uint64_t get_used_memory(void)
{
    uint64_t used = 0;
    for (uint64_t w = 0; w < fa_bitmap_words; w++)
        for (uint64_t b = 0; b < 64; b++)
            if (frame_bitmap[w] & (1ULL << b)) used++;
    return used * PAGE_SIZE;
}

uint64_t get_free_memory(void) { return get_total_memory() - get_used_memory(); }
uint64_t get_free_mem_size(void) { return get_free_memory(); }
