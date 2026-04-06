#pragma once
#include <stdint.h>
#include <stddef.h>

#define KHEAP_START      0xC0000000u
#define KHEAP_INIT_PAGES 1024u
#define KHEAP_INITIAL    (KHEAP_INIT_PAGES * 4096u)

void  init_kheap(void);
void *kmalloc(uint32_t size);
void  kfree(void *ptr);
void  get_kheap_info(size_t *total, size_t *used, size_t *free_bytes);
