segment	.text
align	4
f:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
	pop	eax
	cmp	eax, byte 0
	je	near _L1
