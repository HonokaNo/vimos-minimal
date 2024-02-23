#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "uefi_graphics.h"

void put_serial(char *s);

void put_console(char *s){
  extern int width, height;
  static int x = 0, y = 0;
  static int tmpx = -1, tmpy = -1;
  static uint8_t r = 0xff, g = 0xff, b = 0xff;

  while(*s){
    if(x != width){
      if(*s != '\t' && *s != '\r' && *s != '\n' && *s != '\e' && *s != 0x08){
        put_char((unsigned char)*s, x, y, r, g, b);
        x += 8;

        if(x == (width / 8) * 8){
          x = 0;
          y += 16;
          if(y == (height / 16) * 16){
            scroll_screen();
            x = 0;
            y -= 16;
          }
        }
      }
    }

    if(*s == '\t') x += (x / 8) % 2 ? 8 : 16;
    if(*s == 0x08){
      if(x >= 8){
        x -= 8;
        /* print black rectangle */
        put_char(0xdb, x, y, 0, 0, 0);
      }
    }
    if(*s == '\n'){
      if(tmpx != -1) x = tmpx;
      if(tmpy != -1) y = tmpy - 16;  /* reset line */
      tmpx = -1;
      tmpy = -1;

      r = g = b = 0xff;
      x = 0;
      y += 16;
      if(y == (height / 16) * 16){
        scroll_screen();
        x = 0;
        y -= 16;
      }
    }
    /* ANSI escape sequence */
    if(*s == '\e' && s[1] == '['){
      if(isdigit(s[2])){
        int num = s[2] - '0';

        if(s[3] == 'A'){
          if(tmpy == -1) tmpy = y;
          y -= 16 * num;
          if(y < 0) y = 0;
          s += 1;
        }else if(s[3] == 'B'){
          if(tmpy == -1) tmpy = y;
          y += 16 * num;
          if(y > (height / 16) * 16) y = (height / 16) * 16 - 16;
          s += 1;
        }else if(s[3] == 'C'){
          if(tmpx == -1) tmpx = x;
          x += 8 * num;
          if(x > (width / 8) * 8) x = (width / 8) * 8 - 8;
          s += 1;
        }else if(s[3] == 'D'){
          if(tmpx == -1) tmpx = x;
          x -= 8 * num;
          if(x < 0) x = 0;
          s += 1;
        }else if(num == 3 && s[4] == 'm'){
          int cc = s[3] - '0';
          switch(cc){
            case 0:
              r = g = b = 0x00;
              break;
            case 1:
              r = 0xff;
              g = b = 0x00;
              break;
            case 2:
              g = 0xff;
              r = b = 0x00;
              break;
            case 3:
              r = g = 0xff;
              b = 0x00;
              break;
            case 4:
              b = 0xff;
              r = g = 0x00;
              break;
            case 5:
              r = b = 0xff;
              g = 0xff;
              break;
            case 6:
              g = b = 0xff;
              r = 0x00;
              break;
            case 7:
              r = g = b = 0xff;
              break;
          }
          s += 2;
        }
        s += 1;
      }
      s += 2;

      continue;
    }

    s++;
  }
}

void init_console(bool useVGA){
	extern void (*print_func)(char *s);

  print_func = put_console;
}
