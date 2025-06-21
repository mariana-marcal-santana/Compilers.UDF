segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
align	4
_L1:
	pop	eax
	cmp	eax, byte 0
	je	near _L3
	pop	eax
	cmp	eax, byte 0
	je	near _L4
align	4
_L5:
	pop	eax
	cmp	eax, byte 0
	je	near _L7
segment	.rodata
align	4
_L8:
	db	0
segment	.text
	push	dword $_L8
	call	prints
	add	esp, 4
	call	println
