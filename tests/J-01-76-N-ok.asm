segment	.text
align	4
f:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
segment	.rodata
align	4
_L1:
	db	"ola", 0
segment	.text
	push	dword $_L1
	call	prints
	add	esp, 4
	call	println
        ;; after body 
	leave
	ret
segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
