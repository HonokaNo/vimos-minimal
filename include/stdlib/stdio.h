#ifndef __VIMOS_STDIO_H__
#define __VIMOS_STDIO_H__

#include "stdarg.h"
#include "stddef.h"

typedef struct filedescstd{
  int fd;
}FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF -1

int fgetc(FILE *fp);

int putchar(int c);
int puts(const char *str);
int fputc(int c, FILE *stream);

int sprintf(char *s, const char *format, ...);
int vsprintf(char *s, const char *format, va_list arg);
int printf(const char *format, ...);

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);

#endif
