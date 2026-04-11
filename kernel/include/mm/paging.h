#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

// Page size: 4KB
#define PAGE_SIZE       4096
#define PAGE_SHIFT      12

// Number of entries per table
#define PML4_ENTRIES    512
#define PDPT_ENTRIES    512
#define PD_ENTRIES      512
#define PT_ENTRIES      512

// x86-64 PTE/PDE/PDPTE/PML4E flags
#define PAGE_PRESENT         0x001   // Present
#define PAGE_WRITABLE        0x002   // Read/Write
#define PAGE_USER            0x004   // User/Supervisor
#define PAGE_WRITE_THROUGH   0x008   // Write-Through
#define PAGE_CACHE_DISABLED  0x010   // Cache Disable
#define PAGE_ACCESSED        0x020   // Accessed
#define PAGE_DIRTY           0x040   // Dirty
#define PAGE_PAT             0x080   // PAT (for large pages)
#define PAGE_GLOBAL          0x100   // Global
#define PAGE_NX              0x8000000000000000ULL // No-Execute (inverted, OR with flags)

// Large page flags
#define PAGE_PS              0x080   // Page Size (=1 for PDE/PDPTE that map 2MB/1GB)

// x86-64 address layout constants
#define KERNEL_VIRT_BASE     0xFFFF800000000000ULL
#define KERNEL_HEAP_START    0xFFFF800100000000ULL
#define USER_VIRT_START      0x0000000000400000ULL

// Page table entry (64-bit)
typedef uint64_t page_entry_t;

// Page tables — use pointer types so indexing via [] works correctly
typedef page_entry_t* pml4_t;
typedef page_entry_t* pdpt_t;
typedef page_entry_t* pd_t;
typedef page_entry_t* pt_t;

// Frame allocator
void init_frame_allocator(uint64_t mem_bytes);
uint64_t alloc_frame(void);
void free_frame(uint64_t frame);

// Paging (re-initialize after boot.asm set up initial tables)
void init_paging(void);

// Page table operations
void map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void map_page_user(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void unmap_page(uint64_t virtual_addr);
uint64_t get_physical_addr(uint64_t virtual_addr);
void flush_tlb(uint64_t virtual_addr);

// Memory info
uint64_t get_total_memory(void);
uint64_t get_used_memory(void);
uint64_t get_free_memory(void);

// Get kernel PML4 (returns pointer to PML4 entries)
uint64_t* get_kernel_pml4(void);

// PML4 address (physical, set by boot.asm)
extern uint64_t pml4_phys;

#endif // PAGING_H
