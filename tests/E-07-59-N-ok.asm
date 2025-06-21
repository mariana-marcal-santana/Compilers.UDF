segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
; integer node: 9
	push	dword 9
	call	printi
	add	esp, 4
	call	println
align	4
_L1:
	pop	eax
	cmp	eax, byte 0
	je	near _L3
