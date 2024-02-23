//#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "asm.h"
#include "console.h"
#include "elf.h"
#include "flags.h"
#include "gdt.h"
#include "interrupt.h"
#include "mem.h"
#include "paging.h"
#include "pci.h"
#include "printk.h"
#include "uefi.h"
#include "uefi_graphics.h"
#include "xhci.h"

uint64_t begin_brk = 0, brk = 0;

void vMain(struct UEFI_MemoryMap *memmap, EFI_GRAPHICS_OUTPUT_PROTOCOL *gop){
  actual_init_paging();

#if USE_GOP
  init_gop(gop);
#else
  init_serial();
#endif
  init_console(USE_GOP);

  printk("Welcome Vim-OS!\n");

  init_mem(memmap);

  init_segments();

/* memory test */
#if TEST_MEM
  void *ptr1 = alloc_mem(100);
  printk("pages:%016X\n", ptr1);

  void *ptr2 = alloc_mem(919);
  printk("pages:%016X\n", ptr2);

  void *ptr3 = alloc_mem(30);
  void *ptr4 = alloc_mem(50);

  free_mem(ptr1, 100);
  free_mem(ptr4, 50);
  free_mem(ptr3, 30);
  free_mem(ptr2, 919);
#endif

  init_interrupt();

  __asm__ __volatile__("sti");

  printk("init_pci\n");
  init_pci();

  char p[2];
  p[1] = 0;

  void usb_process_events(void);
  usb_process_events();

  printk("input>");
  while(1){
    char read_usb_keyboard(void);

    p[0] = read_usb_keyboard();
    printk(p);
  }

  while(1) __asm__ __volatile__("hlt");
}
