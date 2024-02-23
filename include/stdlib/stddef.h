#ifndef __VIMOS_STDDEF_H__
#define __VIMOS_STDDEF_H__

#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef DEF_SIZE_T
#define DEF_SIZE_T
typedef uint32_t size_t;
#endif

#ifndef DEF_WCHAR_T
#define DEF_WCHAR_T
typedef uint16_t wchar_t;
#endif

#endif
