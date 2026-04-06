#include <mm/kheap.h>
#include <stddef.h>   /* size_t */

static uint8_t *heap_ptr = (uint8_t *)KHEAP_START;
static uint8_t *heap_end = (uint8_t *)(KHEAP_START + KHEAP_INITIAL);

void init_kheap(void)
{
    /* paging.c 已经把 0xC0000000 开始的 4MB 全部映射好了
     * 这里只需要重置指针，不做任何内存写操作                */
    heap_ptr = (uint8_t *)KHEAP_START;
    heap_end = (uint8_t *)(KHEAP_START + KHEAP_INITIAL);
}

void *kmalloc(uint32_t size)
{
    if (size == 0) return 0;
    size = (size + 7) & ~7u;            /* 8-byte align */
    if (heap_ptr + size > heap_end) return 0;
    void *p = heap_ptr;
    heap_ptr += size;
    return p;
}

void kfree(void *ptr)
{
    (void)ptr;   /* bump allocator — 暂不释放 */
}

void get_kheap_info(size_t *total, size_t *used, size_t *free_bytes)
{
    if (total)      *total      = KHEAP_INITIAL;
    if (used)       *used       = (size_t)(heap_ptr - (uint8_t *)KHEAP_START);
    if (free_bytes) *free_bytes = (size_t)(heap_end - heap_ptr);
}