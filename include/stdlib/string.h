#ifndef __VIMOS_STRING_H__
#define __VIMOS_STRING_H__

#include "stddef.h"

char *strcpy(char *s1, const char *s2);
char *strncpy(char *s1, const char *s2, size_t n);
void *memcpy(void *buf1, const void *buf2, size_t n);

size_t strlen(const char *s);

char *strcat(char *s1, const char *s2);

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);

void *memset(void *buf, int ch, size_t n);

int strcmp(const char *s1, const char *s2);
int memcmp(const void *buf1, const void *buf2, size_t n);

#endif
