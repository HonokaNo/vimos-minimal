#ifndef __VIMOS_LIB_H__
#define __VIMOS_LIB_H__

#include "uefi.h"

extern EFI_SYSTEM_TABLE *ST;
extern EFI_BOOT_SERVICES *BS;

void Print(CHAR16 *str);
void PrintLn(CHAR16 *str);
void convertint(UINT64 value, CHAR16 *buf, int digit);
void converthex(UINT64 value, CHAR16 *buf, int digit);
void convert8(const CHAR16 *src, CHAR8 *dst, int len);
void EFI_ERROR(EFI_STATUS status, unsigned int line);

#define STATUS_IS_SUCCESS(status) (status == EFI_SUCCESS)

void INT_PRINT(UINT64 num, CHAR16 *buffer, int digit);
void INT_PRINTLN(UINT64 num, CHAR16 *buffer, int digit);
void HEX_PRINT(UINT64 num, CHAR16 *buffer, int digit);
void HEX_PRINTLN(UINT64 num, CHAR16 *buffer, int digit);

#endif
