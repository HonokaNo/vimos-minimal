.file "memcpy.S"

.globl memcpy

.type memcpy,@function

.text
memcpy:
  movq %rdi,%rax
  movq %rdx,%rcx
  cld
  rep movsb
  retq
.Lfunc_end0:
  .size memcpy,.Lfunc_end0-memcpy
  .section ".note.GNU-stack","",@progbits
  .addrsig
