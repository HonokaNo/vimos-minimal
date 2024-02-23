#ifndef __VIMOS_PRINTK_H__
#define __VIMOS_PRINTK_H__

int printk(const char *fmt, ...);

__attribute__((noreturn)) void panic(const char *fmt);

#endif
