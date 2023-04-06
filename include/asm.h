#ifndef __VIMOS_ASM_H__
#define __VIMOS_ASM_H__

#include <stdint.h>

#define cli() __asm__ __volatile__("cli")
#define sti() __asm__ __volatile__("sti")

static inline uint8_t in8(uint16_t port){
  uint8_t data;

  __asm__ __volatile__("inb %%dx,%%al":"=a"(data):"d"(port));
  return data;
}

#define out8(port, data) __asm__ __volatile__("outb %%al,%%dx"::"a"(data), "d"(port))

#define save_rflags(flags) __asm__("pushfq\npopq %0": "=r"(flags))
#define load_rflags(flags) __asm__ __volatile__("pushq %0\npopfq":: "r"(flags))

#endif
