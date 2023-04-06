#ifndef __VIMOS_FCNTL_H__
#define __VIMOS_FCNTL_H__

#include "sys/types.h"

#define O_ACCMODE 0x03

#define O_RDONLY 0x00
#define O_WRONLY 0x01
#define O_RDWR   0x02

#define O_CREAT 0x100

int open(const char *pathname, int flags, mode_t mode);

#endif
