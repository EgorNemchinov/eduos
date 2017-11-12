
.extern bot_half

.text
.global bot_half_tramp
bot_half_tramp:
	push %rax
	push %rcx
	push %rdx
	push %rsi
	push %r8
	push %r9
	push %r10
	push %r11
	/* FIXME vfp, xmm ...*/

	call bot_half

	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rax

	ret
