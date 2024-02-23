#include <stdbool.h>
#include <stddef.h>
#include "asm.h"
#include "interrupt.h"
#include "paging.h"

#define CPUID_APIC 0x200

#define LAPIC_BASE 0x1b

#define LAPIC_TPR (0x80 / 4)
#define LAPIC_EOI (0xb0 / 4)
#define LAPIC_SPURIOUS_VECTOR (0xf0 / 4)

#define LAPIC_X2APIC 0x400
#define LAPIC_ENABLE 0x800

#define IOAPIC_BASE 0xfec00000

#define IOAPIC_REG_TABLE 0x10

#define IOAPIC_REDIR_DISABLE 0x10000

static volatile uint32_t *lapic;
static volatile uint32_t *ioapic;

bool check_apic(void){
  uint32_t edx;
  cpuid(1, NULL, NULL, NULL, &edx);
  return edx & CPUID_APIC;
}

static uint32_t ioapic_read(uint32_t reg){
  ioapic[0] = reg;
  return ioapic[4];
}

static void ioapic_write(uint32_t reg, uint32_t val){
  ioapic[0] = reg;
  ioapic[4] = val;
}

static void ioapic_unmask_interrupt(int irq){
  ioapic_write(IOAPIC_REG_TABLE + 2 * irq + 0, 0x20 + irq);
  ioapic_write(IOAPIC_REG_TABLE + 2 * irq + 1, 0);
}

static void lapic_eoi(__attribute__((unused)) int irq){
  lapic[LAPIC_EOI] = 0;
}

void init_apic(void){
  /* LAPIC initialize */
  uint64_t apic_base = readMSR(LAPIC_BASE) & ~0xfff;
  lapic = (uint32_t *)P2V(apic_base);

  apic_base |= LAPIC_ENABLE;  /* enable bit */
  apic_base &= ~LAPIC_X2APIC;  /* disable x2APIC mode */

  writeMSR(LAPIC_BASE, apic_base);

  lapic[LAPIC_SPURIOUS_VECTOR] = (0x100 | 0xff);

  lapic_eoi(0);

  /* enable APIC interrupt */
  lapic[LAPIC_TPR] = 0;

  /* IOAPIC initialize */
  ioapic = (uint32_t *)P2V(IOAPIC_BASE);
  int max_redirection_entries = ioapic_read(0x01) >> 16 & 0xff;
  /* Comaptible PIC setting */
  for(int i = 0; i < max_redirection_entries; i++){
    ioapic_write(IOAPIC_REG_TABLE + 2 * i + 0, 0x20 + i | IOAPIC_REDIR_DISABLE);
    ioapic_write(IOAPIC_REG_TABLE + 2 * i + 1, 0);
  }

  unmask_interrupt = &ioapic_unmask_interrupt;
  end_of_interrupt = &lapic_eoi;
}
