#include "paging.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "asm.h"
#include "mem.h"
#include "printk.h"

void actual_init_paging(void){
  volatile uint64_t *pml4 = (uint64_t *)P2V(getCR3());

  pml4[0] = 0;
}

uint64_t _v2p(uint64_t v){
  if(v - PAGE_OFFSET < v){
    return v - PAGE_OFFSET;
  }else{
    return v - KERNEL_MAPPING + 0x100000;
  }
}

static PML4E *pointer(PML4E *p){
  return (PML4E *)P2V(p->b.addr << 12);
}

static void setPointer(PML4E *p0, PML4E *p1){
  p0->b.addr = V2P(p1) >> 12;
}

static int part(LinearAddress4L *l, int page_map_level){
  if(page_map_level == 0) return l->b.offset;
  if(page_map_level == 1) return l->b.table;
  if(page_map_level == 2) return l->b.directory;
  if(page_map_level == 3) return l->b.directory_ptr;
  if(page_map_level == 4) return l->b.pml;

  return 0;
}

static void setPart(LinearAddress4L *l, int page_map_level, int value){
  if(page_map_level == 0) l->b.offset = value;
  if(page_map_level == 1) l->b.table = value;
  if(page_map_level == 2) l->b.directory = value;
  if(page_map_level == 3) l->b.directory_ptr = value;
  if(page_map_level == 4) l->b.pml = value;
}

static PML4E *newPageMap(void){
  PML4E *e = (PML4E *)alloc_mem(1);
  if(!e) return NULL;

  memset(e, 0, sizeof(uint64_t) * 512);
  return e;
}

static PML4E *setNewPageMapIfNotPresent(PML4E *entry){
  if(!entry) return NULL;

  /* if present */
  if(entry->b.p) return pointer(entry);

  PML4E *child_map = newPageMap();
  if(!child_map) return NULL;

  setPointer(entry, child_map);
  entry->b.p = 1;

  return child_map;
}

static int setupPageMap(PML4E *page_map, int page_map_level, LinearAddress4L *addr, size_t num_pages, bool rw){
  while(num_pages){
    int entry_index = part(addr, page_map_level);

    PML4E *child_map = setNewPageMapIfNotPresent(&page_map[entry_index]);
    if(!child_map) return -1;

    page_map[entry_index].b.rw = rw;
    page_map[entry_index].b.us = 1;

    if(page_map_level == 1) num_pages--;
    else{
      int num_remain_pages = setupPageMap(child_map, page_map_level - 1, addr, num_pages, rw);
      if(num_remain_pages == -1) return -1;

      num_pages = num_remain_pages;
    }

    if(entry_index == 511) break;

    setPart(addr, page_map_level, entry_index + 1);
    for(int level = page_map_level - 1; level >= 1; level--){
      setPart(addr, level, 0);
    }
  }

  return num_pages;
}

void setupPageMaps(LinearAddress4L *addr, size_t num_pages, bool rw){
  PML4E *pml4 = (PML4E *)P2V(getCR3());

  setupPageMap(pml4, 4, addr, num_pages, rw);
}
