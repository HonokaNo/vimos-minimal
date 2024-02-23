#ifndef __VIMOS_ELF_H__
#define __VIMOS_ELF_H__

#include <stdint.h>

typedef uintptr_t Elf_Addr;
typedef uint64_t  Elf_Off;
typedef uint16_t  Elf_Half;
typedef uint32_t  Elf_Word;
typedef int32_t   Elf_Sword;
typedef uint64_t  Elf_Xword;
typedef int64_t   Elf_Sxword;

#define EI_NIDENT 16

/* e_type */
#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

typedef struct{
  unsigned char e_ident[EI_NIDENT];
  Elf_Half    e_type;
  Elf_Half    e_machine;
  Elf_Word    e_version;
  Elf_Addr    e_entry;
  Elf_Off     e_phoff;
  Elf_Off     e_shoff;
  Elf_Word    e_flags;
  Elf_Half    e_ehsize;
  Elf_Half    e_phentsize;
  Elf_Half    e_phnum;
  Elf_Half    e_shentsize;
  Elf_Half    e_shnum;
  Elf_Half    e_shstrndx;
}Elf_Ehdr;

/* p_type */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_LOOS    0x60000000
#define PT_HIOS    0x6FFFFFFF
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7FFFFFFF

/* p_flags */
#define PF_X        0x00000001
#define PF_W        0x00000002
#define PF_R        0x00000004
#define PF_MASKOS   0x00FF0000
#define PF_MASKPROC 0xFF000000

typedef struct{
  Elf_Half    p_type;
  Elf_Half    p_flags;
  Elf_Off     p_offset;
  Elf_Addr    p_vaddr;
  Elf_Addr    p_paddr;
  Elf_Xword   p_filesz;
  Elf_Xword   p_memsz;
  Elf_Xword   p_align;
}Elf_Phdr;

#endif
