#include <string.h>

char *strncpy(char *s1, const char *s2, size_t n){
  size_t i;

/*  for(i = 0; i < n && s2[i] != '\0'; i++){
    s1[i] = s2[i];
  }

  for(; i < n; i++) s1[i] = '\0';*/
  memcpy(s1, s2, n);
  for(i = strlen(s2); i < n; i++) s1[i] = '\0';

  return s1;
}
