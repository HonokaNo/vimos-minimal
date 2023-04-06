.file "memcpy.S"

.globl memcpy

.type memcpy,@function

.text
memcpy1:
	movq %rdx,%rcx

	rep movsb

	retq
.Lfunc_end0:

memcpy8:
	pushq %rcx

	rep movsq

	popq %rcx
	retq
.Lfunc_end1:

memcpy4:
	pushq %rcx

	rep movsd

	popq %rcx
	retq
.Lfunc_end2:

memcpy:
	pushq %rbp
	movq %rsp,%rbp
	pushq %rdi
	pushq %rsi

	cld

	movq %rdx,%rcx
	shrq $3,%rcx
	callq memcpy8

	shlq $3,%rcx
	subq %rcx,%rdx
	cmp $0,%rdx
	jz .Lend

	movq %rdx,%rcx
	shrq $2,%rcx
	callq memcpy4

	shlq $2,%rcx
	subq %rcx,%rdx
	cmp $0,%rdx
	jz .Lend

	callq memcpy1

.Lend:
	popq %rsi
	popq %rax
	movq %rbp,%rsp
	popq %rbp
	retq

.Lfunc_end3:
	.size memcpy1, .Lfunc_end0-memcpy1
	.size memcpy8, .Lfunc_end1-memcpy8
	.size memcpy4, .Lfunc_end2-memcpy4
	.size memcpy, .Lfunc_end3-memcpy
	.section ".note.GNU-stack", "", @progbits
	.addrsig
