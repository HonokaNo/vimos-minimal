#include "gdt.h"
#include <stdint.h>
#include <string.h>
#include "asm.h"
#include "mem.h"
#include "paging.h"
#include "printk.h"

struct segment_descriptor{
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t type : 4;
  uint8_t s : 1;
  uint8_t dpl : 2;
  uint8_t p : 1;
  uint8_t limit_high : 4;
  uint8_t avl : 1;
  uint8_t l : 1;
  uint8_t db : 1;
  uint8_t g : 1;
  uint8_t base_high;
}__attribute__((packed));

#define GDT_SIZE 7
static struct segment_descriptor gdt[GDT_SIZE];

struct gdtr{
  uint16_t limit;
  void *gdt;
}__attribute__((packed));

static struct gdtr gdtr;

struct tss{
  uint32_t rsvd0;
  uint32_t rsp0_low;
  uint32_t rsp0_high;
  uint32_t rsp1_low;
  uint32_t rsp1_high;
  uint32_t rsp2_low;
  uint32_t rsp2_high;
  uint32_t rsvd1;
  uint32_t rsvd2;
  uint32_t ist1_low;
  uint32_t ist1_high;
  uint32_t ist2_low;
  uint32_t ist2_high;
  uint32_t ist3_low;
  uint32_t ist3_high;
  uint32_t ist4_low;
  uint32_t ist4_high;
  uint32_t ist5_low;
  uint32_t ist5_high;
  uint32_t ist6_low;
  uint32_t ist6_high;
  uint32_t ist7_low;
  uint32_t ist7_high;
  uint32_t rsvd3;
  uint32_t rsvd4;
  uint16_t rsvd5;
  uint16_t iopb;
}__attribute__((packed));

static struct tss tss;

static void setCodeSegment(uint8_t num, uint8_t type, uint8_t privilege_level, uint32_t base, uint32_t limit){
  gdt[num].base_low = base & 0xffff;
  gdt[num].base_mid = (base >> 16) & 0xff;
  gdt[num].base_high = (base >> 24) & 0xff;
  gdt[num].limit_low = limit & 0xffff;
  gdt[num].limit_high = (limit >> 16) & 0x0f;
  gdt[num].l = 1;
  gdt[num].p = 1;
  gdt[num].s = 1;
  gdt[num].g = 1;
  gdt[num].dpl = privilege_level;
  gdt[num].type = type;
}

static void setDataSegment(uint8_t num, uint8_t type, uint8_t privilege_level, uint32_t base, uint32_t limit){
  setCodeSegment(num, type, privilege_level, base, limit);
  gdt[num].l = 0;
  gdt[num].db = 1;
}

static void setSystemSegment(uint8_t num, uint8_t type, uint8_t privilege_level, uint32_t base, uint32_t limit){
  setCodeSegment(num, type, privilege_level, base, limit);
  gdt[num].s = 0;
  gdt[num].l = 0;
}

void init_segments(void){
  memset(&gdt[0], 0, sizeof(gdt));

  setCodeSegment(1, 0b1010, 0, 0, 0xfffff);
  setDataSegment(2, 0b0010, 0, 0, 0xfffff);
  setDataSegment(3, 0b0010, 3, 0, 0xfffff);
  setCodeSegment(4, 0b1010, 3, 0, 0xfffff);

  unsigned char *rsp0_stack = alloc_mem(2);
  if(!rsp0_stack) panic("rsp0_stack is NULL!");
  uint64_t rsp0_stack_addr = (uint64_t)rsp0_stack + 8192 - 8;

  unsigned char *ist1_stack = alloc_mem(2);
  if(!ist1_stack) panic("ist1_stack is NULL!");
  uint64_t ist1_stack_addr = (uint64_t)ist1_stack + 8192 - 8;

  tss.rsp0_low = (rsp0_stack_addr >> 0) & 0xffffffff;
  tss.rsp0_high = (rsp0_stack_addr >> 32) & 0xffffffff;
  tss.ist1_low = (ist1_stack_addr >> 0) & 0xffffffff;
  tss.ist1_high = (ist1_stack_addr >> 32) & 0xffffffff;

  /* TSS */
  uint64_t tss_addr = (uint64_t)&tss;
  setSystemSegment(5, 0b1001, 0, tss_addr & 0xffffffff, sizeof(tss) - 1);
  gdt[6].limit_low = tss_addr >> 32 & 0xffff;
  gdt[6].base_low = tss_addr >> 48 & 0xffff;

  gdtr.limit = sizeof(gdt) - 1;
  gdtr.gdt = &gdt;
  __asm__ __volatile__("lgdt %0"::"m"(gdtr));

  setDSAll(0);
  setCSSS(0x08, 0x10);

  __asm__ __volatile__("ltr %%di"::"D"(5 << 3));
}
