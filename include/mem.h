#ifndef __VIMOS_MEM_H__
#define __VIMOS_MEM_H__

#include <stdint.h>

/**
 * UEFI基準のメモリマップ
 *
 * buffer EFI_MEMORY_DESCRIPTOR配列のポインタ
 * mapsize bufferのサイズ
 * descsize EFI_MEMORY_DESCRIPTORのサイズ
 */
struct UEFI_MemoryMap{
  unsigned char *buffer;
  uint64_t mapsize;
  uint64_t descsize;
};

struct memlist{
  struct memlist *next;
  uint32_t pages;
};

void *alloc_mem(uint32_t pages);
void free_mem(void *ptr, uint32_t pages);
void init_mem(struct UEFI_MemoryMap *memmap);

#endif
