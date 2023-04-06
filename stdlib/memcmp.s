.file "memcmp.S"

.globl memcmp

.type memcmp,@function

.text
memcmp:
	pushq %rbp
	movq %rsp,%rbp
	pushq %rdi
	pushq %rsi

	cld

	movq %rdx,%rcx
	shrq $3,%rcx
	pushq %rcx
	repe cmpsq
	jnz .Lerr08
	popq %rcx

	shlq $3,%rcx
	subq %rcx,%rdx
	cmpq $0,%rdx
	jz .Lerr08
	movq %rdx,%rcx
	repe cmpsb
	jmp .Lend1

.Lerr08:
	popq %rcx
	movq -8(%rdi),%rax
	subq -8(%rsi),%rax
	jmp .Lend
.Lend1:
	movq $0,%rax
	movb -1(%rdi),%al
	subb -1(%rsi),%al
.Lend:
	popq %rsi
	popq %rdi
	movq %rbp,%rsp
	popq %rbp
	retq

.Lfunc_end0:
	.size memcmp,.Lfunc_end0-memcmp
	.section ".note.GNU-stack","",@progbits
	.addrsig
