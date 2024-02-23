#include "asm.h"
#include "interrupt.h"

#define MASTER_PIC_COMMAND 0x0020
#define MASTER_PIC_DATA 0x0021
#define SLAVE_PIC_COMMAND 0x00a0
#define SLAVE_PIC_DATA 0x00a1

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define ICW4_8086 0x01

#define PIC_EOI 0x20

void disable_pic(void){
  out8(SLAVE_PIC_DATA, 0xff);
  out8(MASTER_PIC_DATA, 0xff);
}

static void pic_unmask_interrupt(int irq){
  uint16_t port = MASTER_PIC_DATA;

  if(irq >= 8){
    /* unmask master to slave */
    out8(port, in8(port) & ~(1 << 2));
    port = SLAVE_PIC_DATA;
    irq -= 8;
  }

  out8(port, in8(port) & ~(1 << irq));
}

static void pic_eoi(int irq){
  if(irq >= 8) out8(SLAVE_PIC_COMMAND, PIC_EOI);

  out8(MASTER_PIC_COMMAND, PIC_EOI);
}

void init_pic(void){
  disable_pic();

  /* initialization mode */
  out8(MASTER_PIC_COMMAND, ICW1_INIT | ICW1_ICW4);
  out8(SLAVE_PIC_COMMAND, ICW1_INIT | ICW1_ICW4);
  /* interrupt vector offset */
  out8(MASTER_PIC_DATA, 0x20);
  out8(SLAVE_PIC_DATA, 0x28);
  /* PIC connect state setting */
  out8(MASTER_PIC_DATA, 0b00000100);  /* slave is PIC2 */
  out8(SLAVE_PIC_DATA, 0b00000010);  /* master is slave PIC2(PIC9) */
  /* PIC mode setting */
  out8(MASTER_PIC_DATA, ICW4_8086);
  out8(SLAVE_PIC_DATA, ICW4_8086);

  disable_pic();

  unmask_interrupt = &pic_unmask_interrupt;
  end_of_interrupt = &pic_eoi;
}
