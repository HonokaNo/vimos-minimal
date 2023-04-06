#ifndef __VIMOS_MEM_H__
#define __VIMOS_MEM_H__

#include <stdint.h>

#define PAGE_SIZE 4096

/**
 * UEFI基準のメモリマップ
 *
 * buffer EFI_MEMORY_DESCRIPTOR配列のポインタ
 * mapsize bufferのサイズ
 * descsize EFI_MEMORY_DESCRIPTORのサイズ
 */
struct MemoryMap{
  unsigned char *buffer;
  uint64_t mapsize;
  uint64_t descsize;
};

struct mem_block{
  struct mem_block *next;
  uint32_t pages;
};

void *mem_alloc(uint32_t pages);
void mem_free(void *ptr, uint32_t pages);
void init_mem(struct MemoryMap *memmap);
void show_mem_info(void);

#endif
