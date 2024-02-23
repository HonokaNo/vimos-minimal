#include "gdt-asm.h"
#include "syscall-asm.h"

.file "asm.S"

.globl in8,in32
.globl ins16
.globl out8,out32
.globl outs16
.globl disable, restore
.globl setCR0, getCR0
.globl getCR2
.globl setCR3, getCR3
.globl setCR4, getCR4
.globl setXCR0, getXCR0
.globl _vMain
.globl setDSAll, setCSSS
.globl cpuid
.globl readMSR, writeMSR
.globl rdtsc
.globl callApp
.globl syscall_entry

.extern vMain

.macro SAVE_ALL_REGISTERS
  pushq %rax
  pushq %rbx
  pushq %rcx
  pushq %rdx
  pushq %rdi
  pushq %rsi
  pushq %rbp
.endm

.macro RESTORE_ALL_REGISTERS
  popq %rbp
  popq %rsi
  popq %rdi
  popq %rdx
  popq %rcx
  popq %rbx
  popq %rax
.endm

.macro exception_handler_noerr handler_name
.globl \handler_name
.extern do_\handler_name

.p2align 4,0x90
\handler_name:
  pushq $0  # dummy error code
  SAVE_ALL_REGISTERS
  movq %rsp,%rdi
  call do_\handler_name
  RESTORE_ALL_REGISTERS
  addq $8,%rsp  # skip error code
  iretq
.endm

.macro exception_handler handler_name
.globl \handler_name
.extern do_\handler_name

.p2align 4,0x90
\handler_name:
  SAVE_ALL_REGISTERS
  movq %rsp,%rdi
  call do_\handler_name
  RESTORE_ALL_REGISTERS
  addq $8,%rsp  # skip error code
  iretq
.endm

.text
.p2align 4,0x90
in8:
  movw %di,%dx
  inb %dx,%al
  ret

.p2align 4,0x90
in32:
  movw %di,%dx
  inl %dx,%eax
  ret

.p2align 4,0x90
out8:
  movw %di,%dx
  movw %si,%ax
  outb %al,%dx
  ret

.p2align 4,0x90
out32:
  movw %di,%dx
  movl %esi,%eax
  outl %eax,%dx
  ret

.p2align 4,0x90
ins16:
  cld
  movq %rdx,%rcx
  movq %rdi,%rdx
  movq %rsi,%rdi
  rep insw
  ret

.p2align 4,0x90
outs16:
  cld
  movq %rdx,%rcx
  movq %rdi,%rdx
  movq %rsi,%rdi
  rep outsw
  ret

.p2align 4,0x90
disable:
  pushfq
  cli
  popq %rax
  ret

.p2align 4,0x90
restore:
  cli
  pushq %rdi
  popfq
  ret

.p2align 4,0x90
setCR0:
  movq %rdi,%cr0
  ret

.p2align 4,0x90
getCR0:
  movq %cr0,%rax
  ret

.p2align 4,0x90
getCR2:
  movq %cr2,%rax
  ret

.p2align 4,0x90
setCR3:
  movq %rdi,%cr3
  ret

.p2align 4,0x90
getCR3:
  movq %cr3,%rax
  ret

.p2align 4,0x90
setCR4:
  movq %rdi,%cr4
  ret

.p2align 4,0x90
getCR4:
  movq %cr4,%rax
  ret

.p2align 4,0x90
setXCR0:
  movq %rdi,%rax
  xorq %rcx,%rcx
  xsetbv
  ret

.p2align 4,0x90
getXCR0:
  xorq %rcx,%rcx
  xgetbv
  ret

.p2align 4,0x90
_vMain:
  movabsq $0xffff800000600000,%rsp  # 0xffff800000000000 + 0x500000 + 4096 * 256
  call vMain
1:
  hlt
  jmp 1b

.p2align 4,0x90
setCSSS:
  movl %esi,%ss
  movabsq $.next,%rax
  pushq %rdi
  pushq %rax
  lretq
.next:
  ret

.p2align 4,0x90
setDSAll:
  movl %edi,%ds
  movl %edi,%es
  movl %edi,%fs
  movl %edi,%gs
  ret

.p2align 4,0x90
cpuid:
  pushq %rbp
  movq %rsp,%rbp
  pushq %rbx
  movq %rcx,%r10
  movq %rdx,%r11
  movq %rdi,%rax
  cpuid
  testq %rsi,%rsi
  jz .eax_stored
  movl %eax,(%rsi)
.eax_stored:
  testq %r11,%r11
  jz .ebx_stored
  movl %ebx,(%r11)
.ebx_stored:
  testq %r10,%r10
  jz .ecx_stored
  movl %ecx,(%r10)
.ecx_stored:
  testq %r8,%r8
  jz .edx_stored
  movl %edx,(%r8)
.edx_stored:
  popq %rbx
  movq %rbp,%rsp
  popq %rbp
  ret

.p2align 4,0x90
readMSR:
  movq %rdi,%rcx
  rdmsr
  shlq $32,%rdx
  orq %rdx,%rax
  ret

.p2align 4,0x90
writeMSR:
  movq %rdi,%rcx
  movq %rsi,%rdx
  shrq $32,%rdx
  movl %esi,%eax
  wrmsr
  ret

.p2align 4,0x90
rdtsc:
  rdtsc
  shlq $32, %rdx
  orq %rdx, %rax
  ret

.p2align 4,0x90
callApp:
  pushq $USER_SS
  pushq %rsi
  pushq $0x202
  pushq $USER_CS
  pushq %rdi

  # clear all registers
  xorq %rax,%rax
  xorq %rbx,%rbx
  xorq %rcx,%rcx
  xorq %rdx,%rdx
  xorq %rdi,%rdi
  xorq %rsi,%rsi
  xorq %r8,%r8
  xorq %r9,%r9
  xorq %r10,%r10
  xorq %r11,%r11
  xorq %r12,%r12
  xorq %r13,%r13
  xorq %r14,%r14
  xorq %r15,%r15

  iretq

exception_handler_noerr zero_div
exception_handler_noerr breakpoint
exception_handler_noerr invalid_opcode
exception_handler_noerr device_not_available
exception_handler double_fault
exception_handler general_protection
exception_handler page_fault
exception_handler_noerr floating_point
exception_handler_noerr simd_floating_point
