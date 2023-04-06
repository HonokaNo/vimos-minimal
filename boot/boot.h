#ifndef __VIMOS_BOOT_H__
#define __VIMOS_BOOT_H__

#include "uefi.h"

#ifndef NULL
#define NULL (void *)0
#endif

__attribute__((ms_abi))
EFI_STATUS OpenRootFolder(EFI_HANDLE ImageHandle, EFI_FILE_PROTOCOL **root_dir);
__attribute__((ms_abi))
EFI_STATUS ReadFile(EFI_FILE_PROTOCOL *file, VOID **buffer);

#endif
