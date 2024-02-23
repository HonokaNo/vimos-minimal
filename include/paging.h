#ifndef __VIMOS_PAGING_H__
#define __VIMOS_PAGING_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KERNEL_MAPPING 0xffff800000000000
#define PAGE_OFFSET 0xffff806000000000

#define P2V(addr) (PAGE_OFFSET + (uint64_t)(addr))
//#define V2P(addr) (((uint64_t)(addr) - PAGE_OFFSET) < (uint64_t)(addr) ? ((uint64_t)(addr) - PAGE_OFFSET) : ((uint64_t)(addr) - KERNEL_MAPPING + 0x100000))
#define V2P(addr) _v2p((uint64_t)addr)

typedef union{
  uint64_t data;

  struct{
    uint64_t p : 1;
    uint64_t rw : 1;
    uint64_t us : 1;
    uint64_t pwt : 1;
    uint64_t pcd : 1;
    uint64_t a : 1;
    uint64_t : 5;
    uint64_t r : 1;
    uint64_t addr : 40;
    uint64_t : 11;
    uint64_t xd : 1;
  }__attribute__((packed)) b;
}PML4E;

typedef union{
  uint64_t value;

  struct{
    uint64_t offset : 12;
    uint64_t table : 9;
    uint64_t directory : 9;
    uint64_t directory_ptr : 9;
    uint64_t pml : 9;
    uint64_t : 16;
  }__attribute__((packed)) b;
}LinearAddress4L;

void actual_init_paging(void);
uint64_t _v2p(uint64_t v);
void setupPageMaps(LinearAddress4L *addr, size_t num_pages, bool rw);

#endif
