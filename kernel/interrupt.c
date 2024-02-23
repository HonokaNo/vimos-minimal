#include <stdint.h>
#include <string.h>
#include "apic.h"
#include "flags.h"
#include "gdt.h"
#include "pic.h"
#include "printk.h"

struct interrupt_descriptor{
  uint16_t offset_low;
  uint16_t segment;
  uint8_t ist : 3;
  uint8_t reserve : 5;
  uint8_t type : 4;
  uint8_t reserve2 : 1;
  uint8_t dpl : 2;
  uint8_t p : 1;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t reserve3;
}__attribute__((packed));

#define IDT_SIZE 256
static struct interrupt_descriptor idt[IDT_SIZE];

struct idtr{
  uint16_t limit;
  void *idt;
}__attribute__((packed));

static struct idtr idtr;

void (*unmask_interrupt)(int irq) = NULL;
void (*end_of_interrupt)(int irq) = NULL;

void add_interrupt_handler(uint8_t num, void (*handler)(void)){
  idt[num].offset_low = (uint64_t)handler & 0xffff;
  idt[num].segment = KERNEL_CS;
  idt[num].type = 0b1110;  /* interrupt */
  idt[num].p = 1;
  idt[num].offset_mid = ((uint64_t)handler >> 16) & 0xffff;
  idt[num].offset_high = ((uint64_t)handler >> 32) & 0xffffffff;

#if TEST_INT
  printk("interrupt handler %d registered.\n", num);
#endif
}

void init_interrupt(void){
  /* in asm.S */
  extern void zero_div(void);
  extern void breakpoint(void);
  extern void invalid_opcode(void);
  extern void device_not_available(void);
  extern void double_fault(void);
  extern void general_protection(void);
  extern void page_fault(void);
  extern void floating_point(void);
  extern void simd_floating_point(void);

  memset(idt, 0, sizeof(idt));

  add_interrupt_handler(0x00, &zero_div);
  add_interrupt_handler(0x03, &breakpoint);
  add_interrupt_handler(0x06, &invalid_opcode);
  add_interrupt_handler(0x07, &device_not_available);
  add_interrupt_handler(0x08, &double_fault);
  add_interrupt_handler(0x0d, &general_protection);
  add_interrupt_handler(0x0e, &page_fault);
  add_interrupt_handler(0x10, &floating_point);
  add_interrupt_handler(0x13, &simd_floating_point);
  idt[0x0e].ist = 1;

  idtr.limit = sizeof(idt) - 1;
  idtr.idt = &idt;
  __asm__ __volatile__("lidt %0"::"m"(idtr));

  disable_pic();
  if(check_apic()) init_apic();
  else init_pic();
}
