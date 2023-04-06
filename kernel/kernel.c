#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "asm.h"
#include "mem.h"

#define PORT_COM1 0x3f8

void printk(const char *s, ...){
  va_list va;
  char buf[1024];

  va_start(va, s);
  vsprintf(buf, s, va);
  va_end(va);

  for(int i = 0; buf[i] != 0; i++){
    while(!(in8(PORT_COM1 + 5) & 0x20));

    out8(PORT_COM1, buf[i]);
  }
}

void init_serial_port(void){
  out8(PORT_COM1 + 1, 0x00);
  out8(PORT_COM1 + 3, 0x80);
  out8(PORT_COM1 + 0, 0x03);
  out8(PORT_COM1 + 1, 0x00);
  out8(PORT_COM1 + 3, 0x03);
  out8(PORT_COM1 + 2, 0xC7);
  out8(PORT_COM1 + 4, 0x0B);
  out8(PORT_COM1 + 4, 0x1E);
  out8(PORT_COM1 + 0, 0xAE);

  if(in8(PORT_COM1) != (uint8_t)0xAE){
    while(1);
  }

  out8(PORT_COM1 + 4, 0x0F);
}

void vMain(struct MemoryMap *memmap){
	init_serial_port();
	printk("memmap:%016X buffer:%016X\n", memmap, memmap->buffer);
  printk("mapsize:%d descsize:%d\n", memmap->mapsize, memmap->descsize);

  while(1) __asm__("hlt");
}
