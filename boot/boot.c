#include "boot.h"
#include <stdlib.h>
#include "elf.h"
#include "lib.h"
#include "mem.h"
#define DEF_SIZE_T  // HACK: skip define size_t in include/stdlib/stddef.h
#include "paging.h"
#include "uefi.h"

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
  EFI_ERROR();

  status = BS->OpenProtocol(loaded_image->DeviceHandle, &simple_file_guid, (VOID **)&fs, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  EFI_ERROR();

  return fs->OpenVolume(fs, root_dir);
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL *file, VOID **buffer){
  EFI_STATUS status;
  EFI_GUID file_info_id = EFI_FILE_INFO_ID;

  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  UINT8 file_info_buffer[file_info_size];

  status = file->GetInfo(file, &file_info_id, &file_info_size, file_info_buffer);
  EFI_ERROR();

  EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buffer;
  UINTN filesize = file_info->FileSize;

  status = BS->AllocatePool(EfiLoaderData, filesize, buffer);
  EFI_ERROR();

  status = file->Read(file, &filesize, *buffer);
  return status;
}

void CalcLoadAddressRange(Elf_Ehdr *ehdr, UINT64 *start, UINT64 *end){
  Elf_Phdr *phdr = (Elf_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
  *start = UINT64_MAX;
  *end = 0;

  for(Elf_Half i = 0; i < ehdr->e_phnum; i++){
    if(phdr[i].p_type != PT_LOAD) continue;

    *start = min(*start, phdr[i].p_vaddr);
    *end = max(*end, phdr[i].p_vaddr + phdr[i].p_memsz);
  }

  /* convert physical addr */
  *start -= 0xffff800000000000;
  *end -= 0xffff800000000000;
  *start += 0x100000;
  *end += 0x100000;
}

void CopyLoadSegments(Elf_Ehdr *ehdr){
  Elf_Phdr *phdr = (Elf_Phdr *)((UINT64)ehdr + ehdr->e_phoff);

  for(int i = 0; i < ehdr->e_phnum; i++){
    if(phdr[i].p_type != PT_LOAD) continue;

//    UINT64 vaddr = V2P(phdr[i].p_vaddr);
    UINT64 vaddr = phdr[i].p_vaddr;
    UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;

    /* convert physical addr */
    vaddr -= 0xffff800000000000;
    vaddr += 0x100000;

    memcpy((VOID *)vaddr, (VOID *)segm_in_file, phdr[i].p_filesz);

    UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
    memset((CHAR8 *)vaddr + phdr[i].p_filesz, 0, remain_bytes);
  }
}

UINTN GetMemoryMap(struct UEFI_MemoryMap *memmap, UINT32 mapsize){
  UINT32 j;
  UINTN key;

  memmap->mapsize = mapsize;
  EFI_STATUS status = BS->GetMemoryMap(&memmap->mapsize, (EFI_MEMORY_DESCRIPTOR *)memmap->buffer, &key, &memmap->descsize, &j);
  EFI_ERROR();

  return key;
}

EFI_STATUS EFIAPI BootMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
  EFI_STATUS status;
  UINTN key;

  VOID *kernel_buffer;

  ST = SystemTable;
  BS = ST->BootServices;

  CHAR8 memmap_buf[1024 * 16];
  struct UEFI_MemoryMap memmap = {memmap_buf, sizeof(memmap_buf), 0};

  ST->ConOut->ClearScreen(ST->ConOut);
  PrintLn(L"Hello VimOS.");
  PrintLn(L"");

  EFI_GUID grahics_output_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
  status = BS->LocateProtocol(&grahics_output_guid, NULL, (VOID **)&gop);
  EFI_ERROR();
  /* currently, not support */
  if(gop->Mode->Info->PixelFormat == PixelBitMask || gop->Mode->Info->PixelFormat == PixelBltOnly){
    status = EFI_UNSUPPORTED;
    EFI_ERROR();
  }

  EFI_FILE_PROTOCOL *root_dir;
  status = OpenRootFolder(ImageHandle, &root_dir);
  EFI_ERROR();

  /* カーネルを開く */
  EFI_FILE_PROTOCOL *kernel;
  status = root_dir->Open(root_dir, &kernel, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
  EFI_ERROR();

  status = ReadFile(kernel, &kernel_buffer);
  EFI_ERROR();

  /* ELF解析 */
  Elf_Ehdr *kernel_ehdr = (Elf_Ehdr *)kernel_buffer;
  UINT64 kernel_start_addr, kernel_end_addr;
  CalcLoadAddressRange(kernel_ehdr, &kernel_start_addr, &kernel_end_addr);

  UINTN pages = (kernel_end_addr - kernel_start_addr + 0xfff) >> 12;
  status = BS->AllocatePages(AllocateAddress, EfiUnusableMemory, pages, &kernel_start_addr);
  EFI_ERROR();

  CopyLoadSegments(kernel_ehdr);

  status = BS->FreePool(kernel_buffer);
  EFI_ERROR();

  Elf_Ehdr *kernel_elf = (Elf_Ehdr *)kernel_start_addr;
  UINT64 vMain_addr = kernel_elf->e_entry;

  /* stack = kernel + 4MB */
  UINT64 stack_addr = 0x500000;
  status = BS->AllocatePages(AllocateAddress, EfiUnusableMemory, 256, &stack_addr);
  EFI_ERROR();

  /* Alloc Page Tables */
  UINT64 pml4_addr, pdp_addr, page_dir, actual_page_dir;
  UINT64 kernel_pdp, kernel_page_directory_addr, kernel_page_table_addr;
  UINT64 kernel_pdp2, actual_page_dir2;

  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 1, &pml4_addr);
  EFI_ERROR();
  memset((void *)pml4_addr, 0, 4096 * 1);
  status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &pdp_addr);
  EFI_ERROR();
  memset((void *)pdp_addr, 0, 4096 * 1);
  status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, 128, &page_dir);
  EFI_ERROR();
  memset((void *)page_dir, 0, 4096 * 128);
  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 128, &actual_page_dir);
  EFI_ERROR();
  memset((void *)actual_page_dir, 0, 4096 * 128);
//  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, (512 - 128), &actual_page_dir2);
  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 512, &actual_page_dir2);
  EFI_ERROR();
//  memset((void *)actual_page_dir2, 0, 4096 * (512 - 128));
  memset((void *)actual_page_dir2, 0, 4096 * 512);
  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 1, &kernel_pdp);
  EFI_ERROR();
  memset((void *)kernel_pdp, 0, 4096 * 1);
  status = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &kernel_pdp2);
  EFI_ERROR();
  memset((void *)kernel_pdp2, 0, 4096 * 1);
  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 1, &kernel_page_directory_addr);
  EFI_ERROR();
  memset((void *)kernel_page_directory_addr, 0, 4096 * 1);
  status = BS->AllocatePages(AllocateAnyPages, EfiUnusableMemory, 16, &kernel_page_table_addr);
  EFI_ERROR();
  memset((void *)kernel_page_table_addr, 0, 4096 * 16);

  volatile UINT64 *pml4_table = (UINT64 *)pml4_addr, *pdp_table = (UINT64 *)pdp_addr;
  volatile UINT64 *actual_page_directory = (UINT64 *)actual_page_dir, *page_directory = (UINT64 *)page_dir;

  volatile UINT64 *kernel_pdp_table = (UINT64 *)kernel_pdp, *kernel_page_directory = (UINT64 *)kernel_page_directory_addr;
  volatile UINT64 *kernel_page_table = (UINT64 *)kernel_page_table_addr;

  volatile UINT64 *kernel_pdp_table2 = (UINT64 *)kernel_pdp2;
  volatile UINT64 *actual_page_directory2 = (UINT64 *)actual_page_dir2;

  pml4_table[0] = (UINT64)&pdp_table[0] | 0x003;
  pml4_table[256] = (UINT64)&kernel_pdp_table[0] | 0x003;
  pml4_table[257] = (UINT64)&kernel_pdp_table2[0] | 0x003;

  /* physical mapping(128GB) */
  /* 0xxxxxxxxx -> xxxxxxxx */
  for(uint64_t i = 0; i < 128; i++){
    pdp_table[i] = (UINT64)&page_directory[i * 512] | 0x003;
    for(uint64_t j = 0; j < 512; j++){
      page_directory[i * 512 + j] = (i * 512 * 512 * 4096) + (j * 512 * 4096) | 0x083;
    }
  }

  kernel_pdp_table[0] = (UINT64)&kernel_page_directory[0] | 0x003;
  /* kernel mapping(32MB) */
  /* 0xffff8000xxxxxxxx -> 0x100000 + xxxxxxxx */
  for(uint64_t i = 0; i < 16; i++){
    kernel_page_directory[i] = (UINT64)&kernel_page_table[i * 512] | 0x003;
    for(uint64_t j = 0; j < 512; j++){
      kernel_page_table[i * 512 + j] = 0x100000 + (i * 4096 * 512) + (j * 4096) | 0x003;
    }
  }

  /* physical address mapping(512GB) */
  /* 0xffff806000000000 + xxxxxxxx -> xxxxxxxx */
  for(uint64_t i = 512 - 128; i < 512; i++){
    uint64_t index = i - (512 - 128);

    kernel_pdp_table[i] = (UINT64)&actual_page_directory[index * 512] | 0x003;
    for(uint64_t j = 0; j < 512; j++){
      uint64_t addr = (index * 512 * 512 * 4096) + (j * 512 * 4096);
//      Print(L"addr:0x");
//      convert(addr, buf, 16, 16);
//      PrintLn(buf);
      actual_page_directory[index * 512 + j] = addr | 0x083;
    }
  }

  uint64_t base_addr = (uint64_t)128 * (512 * 512 * 4096);
//  for(uint64_t i = 0; i < (512 - 128); i++){
  for(uint64_t i = 0; i < 512; i++){
    kernel_pdp_table2[i] = (UINT64)&actual_page_directory2[i * 512] | 0x003;
    for(uint64_t j = 0; j < 512; j++){
      uint64_t addr = base_addr + (i * 512 * 512 * 4096) + (j * 512 * 4096);
//      Print(L"addr:0x");
//      convert(addr, buf, 16, 16);
//      PrintLn(buf);
      actual_page_directory2[i * 512 + j] = addr | 0x083;
    }
  }

  PrintLn(L"Booting...");

  /* ExitBootServicesのためにメモリマップを要求 */
  key = GetMemoryMap(&memmap, sizeof(memmap_buf));

  status = BS->ExitBootServices(ImageHandle, key);
  if(status != EFI_SUCCESS){
    /* エラーが起きているがこっそり隠す */

    /* retry */
    key = GetMemoryMap(&memmap, sizeof(memmap_buf));
    status = BS->ExitBootServices(ImageHandle, key);
    /* どうしても起動できなさそうならここでエラーメッセージだして止まる */
    EFI_ERROR();
  }

  void *page_tables = (void *)&pml4_table[0];
  /* set page table */
  __asm__ __volatile__("movq %%rax,%%cr3"::"a"(page_tables));

  typedef __attribute__((sysv_abi)) void Main(struct UEFI_MemoryMap *, EFI_GRAPHICS_OUTPUT_PROTOCOL *);
  Main *vMain = (Main *)vMain_addr;
  vMain((struct UEFI_MemoryMap *)P2V((uint64_t)&memmap), (EFI_GRAPHICS_OUTPUT_PROTOCOL *)P2V(gop));

  /* vMain is noreturn */

  return EFI_SUCCESS;
}
