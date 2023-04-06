#ifndef __VIMOS_UNISTD_H__
#define __VIMOS_UNISTD_H__

#include "stddef.h"
#include "stdint.h"

#ifndef SSIZE_TDEFINED
typedef signed long ssize_t;
#define SSIZE_TDEFINED
#endif

int close(int fd);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const char *buf, size_t count);

void *sbrk(intptr_t incr);

__attribute__((noreturn))
void _exit(int status);

int brk(void *addr);
void *sbrk(intptr_t incr);

#endif
