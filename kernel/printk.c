#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm.h"
#include "flags.h"

void (*print_func)(char *s, int count) = NULL;

int printk(const char *fmt, ...){
  static char buf[PRINTK_BUF_SIZE];
  int i;
  va_list ap;
  uint64_t flags;

  flags = disable();

  va_start(ap, fmt);
  i = vsprintf(buf, fmt, ap);
  va_end(ap);

  if(print_func) print_func(buf, i);

  restore(flags);

  return i;
}

__attribute__((noreturn)) void panic(const char *fmt){
  printk("\e[31mPANIC!: %s\n", fmt);
  while(1) __asm__ __volatile__("hlt");
}
