#ifndef __VIMOS_ASM_H__
#define __VIMOS_ASM_H__

#include <stddef.h>
#include <stdint.h>

extern uint8_t in8(uint16_t port);
extern uint32_t in32(uint16_t port);

extern void ins16(uint16_t port, void *buf, int rep);

extern void out8(uint16_t port, uint8_t data);
extern void out32(uint16_t port, uint32_t data);

extern void outs16(uint16_t port, void *buf, int rep);

extern uint64_t disable(void);
extern void restore(uint64_t flags);

extern void setCR0(uint64_t value);
extern uint64_t getCR0(void);

extern uint64_t getCR2(void);

extern void setCR3(void *page_table);
extern uint64_t getCR3(void);

extern void setCR4(uint64_t value);
extern uint64_t getCR4(void);

extern void setXCR0(uint64_t value);
extern uint64_t getXCR0(void);

extern void setDSAll(uint16_t segment);
extern void setCSSS(uint16_t cs, uint16_t ss);

extern void cpuid(int num, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

extern uint64_t readMSR(uint32_t msr);
extern void writeMSR(uint32_t msr, uint64_t value);

extern uint64_t rdtsc(void);

extern void callApp(uint64_t entry, uint64_t rsp);

extern void syscall_entry(void);

#endif
