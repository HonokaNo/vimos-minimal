#include "lib.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;

void Print(CHAR16 *str){
  ST->ConOut->OutputString(ST->ConOut, str);
}

void PrintLn(CHAR16 *str){
  Print(str);
  Print(L"\r\n");
}

void convertint(UINT64 value, CHAR16 *buf, int digit){
  int j;

  for(j = digit - 1; j >= 0; j--){
    buf[j] = L'0' + value % 10;
    value /= 10;
  }

  buf[digit] = 0x0000;
}

void converthex(UINT64 value, CHAR16 *buf, int digit){
  CHAR16 *ch = L"0123456789ABCDEF";
  int j;

  buf[0] = L'0';
  buf[1] = L'x';

  for(j = 1 + digit; j >= 2; j--){
    buf[j] = ch[value % 16];
    value >>= 4;
  }

  buf[2 + digit] = 0x0000;
}

void convert8(const CHAR16 *src, CHAR8 *dst, int len){
  int k;

  for(k = 0; k < len; k++) dst[k] = (CHAR8)(src[k] & 0xff);
}

void EFI_ERROR(EFI_STATUS status, unsigned int line){
  CHAR16 *msg[15] = {
    L"EFI_LOAD_ERROR",
    L"EFI_INVALID_PARAMETER",
    L"EFI_UNSUPPORTED",
    L"EFI_BAD_BUFFER_SIZE",
    L"EFI_BUFFER_TOO_SMALL",
    L"EFI_NOT_READY",
    L"EFI_DEVICE_ERROR",
    L"EFI_WRITE_PROTECTED",
    L"EFI_OUT_OF_RESOURCES",
    L"EFI_VOLUME_CORRUPTED",
    L"EFI_VOLUME_FULL",
    L"EFI_NO_MEDIA",
    L"EFI_MEDIA_CHANGED",
    L"EFI_NOT_FOUND",
    L"EFI_ACCESS_DENIED"
  };
  CHAR16 s[5];

  if(STATUS_IS_SUCCESS(status)) return;
  else{
    Print(L"Error! ");
    PrintLn(msg[status - 1]);

    Print(L"Line:");
    convertint(line, s, 4);
    PrintLn(s);

    while(1) __asm__("hlt");
  }
}

void INT_PRINT(UINT64 num, CHAR16 *buffer, int digit){
  convertint(num, buffer, digit);
  Print(buffer);
}

void INT_PRINTLN(UINT64 num, CHAR16 *buffer, int digit){
  convertint(num, buffer, digit);
  PrintLn(buffer);
}

void HEX_PRINT(UINT64 num, CHAR16 *buffer, int digit){
  converthex(num, buffer, digit);
  Print(buffer);
}

void HEX_PRINTLN(UINT64 num, CHAR16 *buffer, int digit){
  converthex(num, buffer, digit);
  PrintLn(buffer);
}
