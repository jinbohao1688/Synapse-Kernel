#include <mm/kheap.h>
#include <stddef.h>

static uint8_t* heap_ptr = (uint8_t*)KHEAP_START;
static uint8_t* heap_end = (uint8_t*)(KHEAP_START + KHEAP_INITIAL);

void init_kheap(void)
{
    heap_ptr = (uint8_t*)KHEAP_START;
    heap_end = (uint8_t*)(KHEAP_START + KHEAP_INITIAL);
}

void* kmalloc(size_t size)
{
    if (size == 0) return 0;
    size = (size + 7) & ~7ULL;           /* 8-byte align */
    if (heap_ptr + size > heap_end) return 0;
    void* p = heap_ptr;
    heap_ptr += size;
    return p;
}

void kfree(void* ptr)
{
    (void)ptr;   /* bump allocator — no-op */
}

void get_kheap_info(size_t* total, size_t* used, size_t* free_bytes)
{
    if (total)      *total      = KHEAP_INITIAL;
    if (used)       *used       = (size_t)(heap_ptr - (uint8_t*)KHEAP_START);
    if (free_bytes) *free_bytes = (size_t)(heap_end - heap_ptr);
}
