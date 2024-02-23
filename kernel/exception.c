#include <stdbool.h>
#include "asm.h"
#include "flags.h"
#include "interrupt.h"
#include "paging.h"
#include "printk.h"

void do_zero_div(struct interrupt_frame *regs){
  printk("\e[32m### Zero Division ###\n");
  printk("RIP:%016X\n", regs->rip);

  while(1);
}

void do_breakpoint(struct interrupt_frame *regs){
  printk("\e[32m### Breakpoint ###\n");
  printk("RIP:%016X\n", regs->rip);
}

void do_invalid_opcode(struct interrupt_frame *regs){
  printk("\e[32m### Invalid Opcode ###\n");
  printk("RIP:%016X\n", regs->rip);

  while(1);
}

void do_device_not_available(struct interrupt_frame *regs){
  printk("\e[32m### Device Not Available ###\n");
  printk("RIP:%016X\n", regs->rip);

  while(1);
}

void do_double_fault(struct interrupt_frame *regs){
  printk("\e[32m### Double Fault ###\n");
  printk("RIP:%016X\n", regs->rip);
  printk("Error:%016X\n", regs->error);

  while(1);
}

void do_general_protection(struct interrupt_frame *regs){
  printk("\e[32m### General Protection ###\n");
  printk("RIP:%016X\n", regs->rip);
  printk("Error:%016X\n", regs->error);

  while(1);
}

void do_page_fault(struct interrupt_frame *regs){
#if TEST_PAGE_FAULT
  printk("do_page_fault\n");
  printk("rip:%016X cr2:%016X\n", regs->rip, getCR2());
#endif

  if(regs->error & 0x01){
    printk("\e[32m### Page Fault ###\n");
    printk("RIP:%016X\n", regs->rip);
    printk("CR2:%016X\n", getCR2());
    printk("Error:%016X\n", regs->error);

    while(1);
  }

  extern uint64_t brk;

  uint64_t cr2 = getCR2();
#if TEST_PAGE_FAULT
  printk("%016X <= %016X <= %016X\n", brk, cr2, 0x7fffffffe000);
#endif
  /* brk-start_stack */
  if(brk <= cr2 && cr2 <= 0x7fffffffe000){
    LinearAddress4L addr = {cr2};
    setupPageMaps(&addr, 1, true);
  }
}

void do_floating_point(struct interrupt_frame *regs){
  printk("\e[32m### x87 Floating Point ###\n");
  printk("RIP:%016X\n", regs->rip);

  while(1);
}

void do_simd_floating_point(struct interrupt_frame *regs){
  printk("\e[32m### SIMD Floating Point ###\n");
  printk("RIP:%016X\n", regs->rip);

  while(1);
}
