#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s){
  int i = 0;

  while(is_digit(**s)) i = i * 10 + *((*s)++) - '0';

  return i;
}

#define ZEROPAD 1
#define SIGN 2
#define PLUS 4
#define SPACE 8
#define LEFT 16
#define SPECIAL 32
#define SMALL 64

static char *number(char *str, unsigned long num, int base, int size, int precision, int type){
  char c, sign, tmp[36];
  const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i = 0;

  if(type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  if(type & LEFT) type &= ~ZEROPAD;
  if(base < 2 || base > 36) return 0;

  c = (type & ZEROPAD) ? '0' : ' ';
  if(type & SIGN && num < 0){
    sign = '-';
    num = -num;
  }else sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
  if(sign) size--;

  if(num == 0) tmp[i++] = '0';
  else{
    while(num != 0){
      tmp[i++] = digits[num % base];
      num /= base;
    }
  }

  if(i > precision) precision = i;
  size -= precision;
  if(!(type & (ZEROPAD + LEFT))) while(size-- > 0) *str++ = ' ';
  if(sign) *str++ = sign;

  if(type & SPECIAL){
    if(base == 8) *str++ = '0';
    else if(base == 16){
      *str++ = '0';
      *str++ = digits[33];
    }
  }
  if(!(type & LEFT)) while(size-- > 0) *str++ = c;

  while(i < precision--) *str++ = '0';
  while(i-- > 0) *str++ = tmp[i];
  while(size-- > 0) *str++ = ' ';

  return str;
}

int vsprintf(char *buf, const char *fmt, va_list args){
  int len, i;
  char *str, *s;
  int flags, field_width, precision, qualifier;

  for(str = buf; *fmt; fmt++){
    if(*fmt != '%'){
      *str++ = *fmt;
      continue;
    }

    flags = 0;
repeat:
    ++fmt;
    switch(*fmt){
      case '-': flags |= LEFT; goto repeat;
      case '+': flags |= PLUS; goto repeat;
      case ' ': flags |= SPACE; goto repeat;
      case '#': flags |= SPECIAL; goto repeat;
      case '0': flags |= ZEROPAD; goto repeat;
    }

    field_width = -1;
    if(is_digit(*fmt)) field_width = skip_atoi(&fmt);
    else if(*fmt == '*'){
      field_width = va_arg(args, int);
      if(field_width < 0){
        field_width = -field_width;
        flags |= LEFT;
      }
    }

    precision = -1;
    if(*fmt == '.'){
      ++fmt;
      if(is_digit(*fmt)) precision = skip_atoi(&fmt);
      else if(*fmt == '*') precision = va_arg(args, int);

      if(precision < 0) precision = 0;
    }

    qualifier = -1;
    if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L'){
      qualifier = *fmt;
      ++fmt;
    }

    switch(*fmt){
      case 's':
        s = va_arg(args, char *);
        len = strlen(s);

        if(precision < 0) precision = len;
        else if(len > precision) len = precision;

        if(!(flags & LEFT)){
          while(len < field_width--) *str++ = ' ';
        }
        for(i = 0; i < len; i++) *str++ = *s++;

        while(len < field_width--) *str++ = ' ';
        break;
      case 'x':
        flags |= SMALL;
      case 'X':
        str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
        break;
      case 'd':
      case 'i':
        flags |= SIGN;
      case 'u':
        str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
        break;
      default:
        if(*fmt != '%') *str++ = '%';
        if(*fmt) *str++ = *fmt;
        else --fmt;

        break;
    }
  }

  *str = '\0';
  return str-buf;
}
