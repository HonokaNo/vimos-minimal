#include <string.h>

char *strrchr(const char *s, int c){
  char *p = NULL;

  do{
    if(*s == c) p = (char *)s;
  }while(*s++);

  return p;
}
