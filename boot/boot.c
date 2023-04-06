#include "boot.h"
#include "elf.h"
#include "lib.h"
#include "mem.h"
#include "platform.h"
#include "uefi.h"

#define READ_MAX_SIZE 256 * 1024 * 1024

void *memcpy(void *buf1, const void *buf2, size_t n){
  UINT8 *x = (UINT8 *)buf1;
  const UINT8 *y = (const UINT8 *)buf2;

  while(n--){
    *(x++) = *(y++);
  }

  return buf1;
}

void *memset(void *buf, int ch, size_t n){
  unsigned char *s = buf;
  while(n--) *s++ = (unsigned char)ch;

  return buf;
}

EFI_STATUS OpenRootFolder(EFI_HANDLE ImageHandle, EFI_FILE_PROTOCOL **root_dir){
  EFI_STATUS status;
  EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
  EFI_GUID simple_file_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

  EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

  status = BS->OpenProtocol(ImageHandle, &loaded_image_guid, (VOID **)&loaded_image, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  EFI_ERROR(status, __LINE__);

  status = BS->OpenProtocol(loaded_image->DeviceHandle, &simple_file_guid, (VOID **)&fs, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  EFI_ERROR(status, __LINE__);

  return fs->OpenVolume(fs, root_dir);
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL *file, VOID **buffer){
  EFI_STATUS status;
  EFI_GUID file_info_id = EFI_FILE_INFO_ID;

  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];

  status = file->GetInfo(file, &file_info_id, &file_info_size, file_info_buffer);
  EFI_ERROR(status, __LINE__);

  EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
  UINTN filesize = file_info->FileSize;

  status = BS->AllocatePool(EfiLoaderData, filesize, buffer);
  EFI_ERROR(status, __LINE__);

  status = file->Read(file, &filesize, *buffer);
  return status;
}

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

void CalcLoadAddressRange(Elf_Ehdr *ehdr, UINT64 *start, UINT64 *end){
  Elf_Phdr *phdr = (Elf_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
  *start = UINT64_MAX;
  *end = 0;

  for(Elf_Half i = 0; i < ehdr->e_phnum; i++){
    if(phdr[i].p_type != PT_LOAD) continue;

    *start = MIN(*start, phdr[i].p_vaddr);
    *end = MAX(*end, phdr[i].p_vaddr + phdr[i].p_memsz);
  }
}

void CopyLoadSegments(Elf_Ehdr *ehdr){
  Elf_Phdr *phdr = (Elf_Phdr *)((UINT64)ehdr + ehdr->e_phoff);

  for(Elf_Half i = 0; i < ehdr->e_phnum; i++){
    if(phdr[i].p_type != PT_LOAD) continue;

    UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
    memcpy((VOID *)phdr[i].p_vaddr, (VOID *)segm_in_file, phdr[i].p_filesz);

    UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
    memset((CHAR8 *)phdr[i].p_vaddr + phdr[i].p_filesz, 0, remain_bytes);
  }
}

UINTN GetMemoryMap(struct MemoryMap *memmap, UINT32 mapsize){
  UINT32 j;
  UINTN key;

  memmap->mapsize = mapsize;
  EFI_STATUS status = BS->GetMemoryMap(&memmap->mapsize, (EFI_MEMORY_DESCRIPTOR *)memmap->buffer, &key, &memmap->descsize, &j);
  EFI_ERROR(status, __LINE__);

  return key;
}

EFI_STATUS EFIAPI BootMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
  EFI_STATUS status;
  UINTN key;

  VOID *kernel_buffer;

  ST = SystemTable;
  BS = ST->BootServices;

  CHAR8 memmap_buf[1024 * 16];
  struct MemoryMap memmap = {memmap_buf, sizeof(memmap_buf), 0};

  ST->ConOut->ClearScreen(ST->ConOut);
  PrintLn(L"Hello VimOS.");
  PrintLn(L"");

  EFI_FILE_PROTOCOL *root_dir;
  status = OpenRootFolder(ImageHandle, &root_dir);
  EFI_ERROR(status, __LINE__);

  /* カーネルを開く */
  EFI_FILE_PROTOCOL *kernel;
  status = root_dir->Open(root_dir, &kernel, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
  EFI_ERROR(status, __LINE__);

  status = ReadFile(kernel, &kernel_buffer);
  EFI_ERROR(status, __LINE__);

  /* ELF解析 */
  Elf_Ehdr *kernel_ehdr = (Elf_Ehdr *)kernel_buffer;
  UINT64 kernel_start_addr, kernel_end_addr;
  CalcLoadAddressRange(kernel_ehdr, &kernel_start_addr, &kernel_end_addr);

  UINTN pages = (kernel_end_addr - kernel_start_addr + 0xfff) >> 12;
  status = BS->AllocatePages(AllocateAddress, EfiUnusableMemory, pages, &kernel_start_addr);
  EFI_ERROR(status, __LINE__);

  CopyLoadSegments(kernel_ehdr);

  status = BS->FreePool(kernel_buffer);
  EFI_ERROR(status, __LINE__);

  Elf_Ehdr *kernel_elf = (Elf_Ehdr *)kernel_start_addr;
  UINT64 vMain_addr = kernel_elf->e_entry;

  /* stack = kernel + 4MB */
  UINT64 stack_addr = 0x500000;
  status = BS->AllocatePages(AllocateAddress, EfiUnusableMemory, 256, &stack_addr);
  EFI_ERROR(status, __LINE__);

  PrintLn(L"Booting...");

  /* ExitBootServicesのためにメモリマップを要求 */
  key = GetMemoryMap(&memmap, sizeof(memmap_buf));
  CHAR16 s[20];
  convertint(memmap.mapsize, &s[0], 3);
  PrintLn(&s[0]);
  convertint(memmap.descsize, &s[0], 3);
  PrintLn(&s[0]);
  converthex((UINT64)&memmap, &s[0], 16);
  PrintLn(&s[0]);

  status = BS->ExitBootServices(ImageHandle, key);
  if(status != EFI_SUCCESS){
    /* エラーが起きているがこっそり隠す */

    /* retry */
    key = GetMemoryMap(&memmap, sizeof(memmap_buf));
    status = BS->ExitBootServices(ImageHandle, key);
    /* どうしても起動できなさそうならここでエラーメッセージだして止まる */
    EFI_ERROR(status, __LINE__);
  }

  UINT64 stack_pointer = stack_addr + (256 * 4096);
  __asm__("movq %0, %%rsp"::"r"(stack_pointer));

  typedef void Main(const struct MemoryMap *);
  Main *vMain = (Main *)vMain_addr;
  vMain(&memmap);

  while(1);
}
