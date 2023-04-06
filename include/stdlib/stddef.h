#ifndef __VIMOS_STDDEF_H__
#define __VIMOS_STDDEF_H__

#define NULL ((void *)0)

typedef unsigned int size_t;
/* wchar_t is 2 bytes */
typedef unsigned short wchar_t;

#define offsetof(type, mem) ((size_t)((char *)&((type *)0)->mem - (char *)(type *)0))

#endif
