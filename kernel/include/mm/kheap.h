#pragma once
#include <stdint.h>
#include <stddef.h>

// x86-64 kernel heap: starts at 0xFFFF800100000000 (1GB into kernel space)
#define KHEAP_START      0xFFFF800100000000ULL
#define KHEAP_INIT_PAGES 256u
#define KHEAP_INITIAL    (KHEAP_INIT_PAGES * 4096ULL)

void  init_kheap(void);
void* kmalloc(size_t size);
void  kfree(void* ptr);
void  get_kheap_info(size_t* total, size_t* used, size_t* free_bytes);
