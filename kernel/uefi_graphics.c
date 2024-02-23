#include "uefi_graphics.h"
#include <stdio.h>
#include <string.h>
#include "paging.h"
#include "printk.h"
#include "uefi.h"

int width, height;
static int pixels_per_scanline;
static unsigned char *frame_buffer;

void putPixel_RGB(int x, int y, uint8_t r, uint8_t g, uint8_t b){
  if(x >= width || y >= height) return;

  frame_buffer[(y * pixels_per_scanline + x) * 4 + 0] = r;
  frame_buffer[(y * pixels_per_scanline + x) * 4 + 1] = g;
  frame_buffer[(y * pixels_per_scanline + x) * 4 + 2] = b;
}

void putPixel_BGR(int x, int y, uint8_t r, uint8_t g, uint8_t b){
  if(x >= width || y >= height) return;

  frame_buffer[(y * pixels_per_scanline + x) * 4 + 0] = b;
  frame_buffer[(y * pixels_per_scanline + x) * 4 + 1] = g;
  frame_buffer[(y * pixels_per_scanline + x) * 4 + 2] = r;
}

static void (*putPixel)(int x, int y, uint8_t r, uint8_t g, uint8_t b) = &putPixel_RGB;

extern unsigned char font8x16[4096];

void put_char(unsigned char c, int x, int y, uint8_t r, uint8_t g, uint8_t b){
  for(int i = 0; i < 16; i++){
    unsigned char f = font8x16[c * 16 + i];

    if(f & 0x80) putPixel(x + 0, y + i, r, g, b);
    if(f & 0x40) putPixel(x + 1, y + i, r, g, b);
    if(f & 0x20) putPixel(x + 2, y + i, r, g, b);
    if(f & 0x10) putPixel(x + 3, y + i, r, g, b);
    if(f & 0x08) putPixel(x + 4, y + i, r, g, b);
    if(f & 0x04) putPixel(x + 5, y + i, r, g, b);
    if(f & 0x02) putPixel(x + 6, y + i, r, g, b);
    if(f & 0x01) putPixel(x + 7, y + i, r, g, b);
  }
}

/* need fix for Blt mode? */
void clear_screen(void){
  /* reset buffer */
  for(int i = 0; i < height; i++){
    for(int j = 0; j < width; j++){
      putPixel(j, i, 0x00, 0x00, 0x00);
    }
  }
}

void scroll_screen(void){
#if 0
  memcpy(frame_buffer, frame_buffer + pixels_per_scanline * 16 * 4, pixels_per_scanline * (height - 16) * 4);
#else
  for(int i = 0; i < height - 16; i++){
    memcpy(frame_buffer + (i * pixels_per_scanline * 4), frame_buffer + ((i + 16) * pixels_per_scanline * 4), 400 * 4);
  }
#endif
//  memset(frame_buffer + pixels_per_scanline * (height - 16) * 4, 0, width * 16 * 4);
  for(int i = 0; i < 16; i++){
    memset(frame_buffer + pixels_per_scanline * (height - i) * 4, 0, 400 * 16 * 4);
  }
}

void put_string(unsigned char *s, int x, int y, uint8_t r, uint8_t g, uint8_t b){
  while(*s){
    if(*s == '\n'){
      y += 16;
      x = 0;
      s++;
    }
    if(x != width){
      put_char(*s, x, y, r, g, b);
      x += 8;
    }

    s++;
  }
}

void init_gop(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop){
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *)P2V((uint64_t)gop->Mode);
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)P2V((uint64_t)mode->Info);

  width = info->HorizontalResolution;
  height = info->VerticalResolution;
  pixels_per_scanline = info->PixelsPerScanLine;
  frame_buffer = (unsigned char *)P2V(mode->FrameBufferBase);

  if(info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor){
    putPixel = &putPixel_BGR;
  }

  clear_screen();
}
