#include <string.h>

char *strcat(char *s1, const char *s2){
  int i = 0;

  while(s1[i]) i++;

  strcpy(&s1[i], s2);

  return s1;
}
