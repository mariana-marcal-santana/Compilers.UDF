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
align	4
_L4:
	pop	eax
	cmp	eax, byte 0
	je	near _L6
	pop	eax
	cmp	eax, byte 0
	je	near _L7
	jmp	dword _L6
	jmp	dword _L8
_L7:
_L8:
