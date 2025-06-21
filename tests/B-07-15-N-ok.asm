segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
; integer node: 150
	push	dword 150
; integer node: 100
	push	dword 100
	pop	ecx
	pop	eax
	cdq
	idiv	ecx
	push	edx
	call	printi
	add	esp, 4
	call	println
        ;; after body 
	leave
	ret
extern	printi
extern	println
