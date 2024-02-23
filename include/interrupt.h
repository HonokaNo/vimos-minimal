#ifndef __VIMOS_INTERRUPT_H__
#define __VIMOS_INTERRUPT_H__

#include <stdint.h>

struct interrupt_frame{
  uint64_t rbp, rsi, rdi;
  uint64_t rdx, rcx, rbx, rax;
  uint64_t error, rip, rsp, rflags;
}__attribute__((packed));

extern void (*unmask_interrupt)(int irq);
extern void (*end_of_interrupt)(int irq);

void add_interrupt_handler(uint8_t num, void (*handler)(void));
void init_interrupt(void);

#endif
