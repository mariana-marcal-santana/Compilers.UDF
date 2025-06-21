segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
segment	.rodata
align	4
_L1:
	db	"X", 32, 10, 32, "Y", 32, 10, 32, "Z", 32, 10, 0
segment	.text
	push	dword $_L1
	call	prints
	add	esp, 4
	call	println
        ;; after body 
	leave
	ret
extern	println
extern	prints
