#include <string.h>
#include <stddef.h>

void *memset(void *buf, int ch, size_t n){
  size_t i;
  char *p = buf;

  for(i = 0; i < n; i++) p[i] = ch;

  return buf;
}
