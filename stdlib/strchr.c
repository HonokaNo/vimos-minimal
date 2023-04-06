#include <string.h>
#include <stddef.h>

char *strchr(const char *s, int c){
  int i;

  for(i = 0; s[i] != 0; i++){
    if(s[i] == c) return (char *)&s[i];
  }

  /* if c == 0, check */
  if(s[i] == c) return (char *)&s[i];
  else return NULL;
}
