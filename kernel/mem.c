#include "mem.h"
#include <stddef.h>
#include "flags.h"
#include "paging.h"
#include "printk.h"
#include "uefi.h"

static struct memlist memlist;

void *alloc_mem(uint32_t pages){
  struct memlist *m, *prev = &memlist;
  void *ptr = NULL;

  for(m = memlist.next; m; prev = m, m = m->next){
//    printk("alloc:Start:%016X  pages:%d\n", (uint64_t)m, m->pages);

    if(m->pages == pages){
      ptr = m;
      prev->next = m->next;

      break;
    }else if(m->pages >= pages){
      ptr = m;

      struct memlist *ml = (struct memlist *)((uint64_t)m + 4096 * pages);
      prev->next = ml;
      ml->next = m->next;
      ml->pages = m->pages - pages;

      break;
    }
  }

#if TEST_MEM
  printk("\n");
  for(m = memlist.next; m; m = m->next){
    printk("Start:%016X  size:%d\n", (uint64_t)m, m->pages);
  }
#endif

  return ptr;
}

void free_mem(void *ptr, uint32_t pages){
  struct memlist *m, *prev = &memlist;

//  printk("\n");
  for(m = memlist.next; m; prev = m, m = m->next){
//    printk("Start:%016X  pages:%d\n", (uint64_t)m, m->pages);
//    printk("ptr Start:%016X  End:%016X\n", (uint64_t)ptr, (uint64_t)ptr + 4096 * pages);

    /* not merged any block(before all memlists) */
    if(prev == &memlist && (uint64_t)ptr < (uint64_t)m){
      struct memlist *ml = (struct memlist *)ptr;
      prev->next = ml;
      ml->next = m;
      ml->pages = pages;

      m = ml;
    }

    /* not merged any block */
    if(prev != &memlist && (uint64_t)prev + 4096 * prev->pages < (uint64_t)ptr){
      struct memlist *ml = (struct memlist *)ptr;
      ml->next = prev->next;
      prev->next = ml;
      ml->pages = pages;

      m = ml;
    }

    /* merge next block */
    if((uint64_t)ptr + 4096 * pages == (uint64_t)m){
      struct memlist *ml = (struct memlist *)ptr;
      prev->next = ml;
      ml->next = m->next;
      ml->pages = pages + m->pages;

      m = prev->next;
    }

    /* merge prev block */
    if((uint64_t)m + 4096 * m->pages == (uint64_t)ptr){
      m->pages += pages;
    }
  }

#if TEST_MEM
  printk("\n");
  for(m = memlist.next; m; m = m->next){
    printk("Start:%016X  size:%d\n", (uint64_t)m, m->pages);
  }
#endif
}

static int isUsableMemType(uint16_t type){
  return type == EfiLoaderCode || type == EfiLoaderData || type == EfiBootServicesCode || type == EfiBootServicesData || type == EfiConventionalMemory;
}

void init_mem(struct UEFI_MemoryMap *memmap){
  void *base = (void *)P2V((uint64_t)memmap->buffer), *m;
  uint64_t start = 0, pages = 0;
  struct memlist *cur_memlist = &memlist;

#if TEST_MEM
  printk("buffer:%016X\n", memmap->buffer);
  printk("mapsize:%d\n", memmap->mapsize);
  printk("descsize:%d\n", memmap->descsize);
#endif
#if 0
  printk("Base             VBase            Length           Type\n");
#endif

  for(m = base; m < base + memmap->mapsize; m += memmap->descsize){
    EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)m;

#if 0
    printk("%016X %016X %016X %08X\n", desc->PhysicalStart, desc->VirtualStart, desc->NumberOfPage * 4096, desc->Type);
#endif
    if(isUsableMemType(desc->Type)){
      if(start == 0){
        start = desc->PhysicalStart;
        pages = desc->NumberOfPage;
      }
      if(start + pages * 4096 == desc->PhysicalStart){
        pages += desc->NumberOfPage;
      }
    }else if(start != 0){
#if TEST_MEM
      printk("write desc:(%016X, %d)\n", start, pages);
#endif
      struct memlist *ml = (struct memlist *)P2V(start);
      ml->next = NULL;
      ml->pages = pages;
      /* count free pages */
      memlist.pages += pages;
      /* set next memlist */
      cur_memlist->next = ml;
      cur_memlist = ml;

      start = 0;
    }
  }

#if TEST_MEM
  for(cur_memlist = memlist.next; cur_memlist != NULL; cur_memlist = cur_memlist->next){
    printk("Start:%016X  size:%d\n", (uint64_t)cur_memlist, cur_memlist->pages);
  }
#endif
}
