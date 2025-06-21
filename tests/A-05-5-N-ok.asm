segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
; integer node: 10
	push	dword 10
	call	printi
	add	esp, 4
	call	println
        ;; after body 
	leave
	ret
extern	printi
extern	println
