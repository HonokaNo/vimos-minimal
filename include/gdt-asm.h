#ifndef __VIMOS_GDT_ASM_H__
#define __VIMOS_GDT_ASM_H__

#define KERNEL_CS 0x08
#define KERNEL_SS 0x10
#define USER_SS (0x18 | 0x03)
#define USER_CS (0x20 | 0x03)

#endif
