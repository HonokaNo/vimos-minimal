#include <stdarg.h>
#include <stdio.h>

int sprintf(char *s, const char *format, ...){
  va_list va;
  int i;

  va_start(va, format);

  i = vsprintf(s, format, va);

  va_end(va);

  return i;
}
