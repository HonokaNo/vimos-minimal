#ifndef __VIMOS_UEFI_GRAPHICS_H__
#define __VIMOS_UEFI_GRAPHICS_H__

#include "uefi.h"

/*struct display{
  int width, height;
  int pixels_per_scanline;
  unsigned char *frame_buffer;
  int pixel_format;
};*/

void init_gop(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);
void put_char(unsigned char c, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void put_string(unsigned char *s, int x, int y, uint8_t r, uint8_t g, uint8_t b);
void clear_screen(void);
void scroll_screen(void);

#endif
