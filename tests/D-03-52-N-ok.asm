segment	.text
align	4
global	_main:function
_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
        ;; before body 
; integer node: 0
	push	dword 0
	pop	eax
	cmp	eax, byte 0
	je	near _L1
segment	.rodata
align	4
_L2:
	db	"KO", 0
segment	.text
	push	dword $_L2
	call	prints
	add	esp, 4
	call	println
	jmp	dword _L3
_L1:
; integer node: 0
	push	dword 0
	pop	eax
	cmp	eax, byte 0
	je	near _L4
segment	.rodata
align	4
_L5:
	db	"elif1", 0
segment	.text
	push	dword $_L5
	call	prints
	add	esp, 4
	call	println
	jmp	dword _L6
_L4:
; integer node: 0
	push	dword 0
	pop	eax
	cmp	eax, byte 0
	je	near _L7
segment	.rodata
align	4
_L8:
	db	"elif2", 0
segment	.text
	push	dword $_L8
	call	prints
	add	esp, 4
	call	println
	jmp	dword _L9
_L7:
; integer node: 0
	push	dword 0
	pop	eax
	cmp	eax, byte 0
	je	near _L10
segment	.rodata
align	4
_L11:
	db	"elif3", 0
segment	.text
	push	dword $_L11
	call	prints
	add	esp, 4
	call	println
	jmp	dword _L12
_L10:
; integer node: 0
	push	dword 0
	pop	eax
	cmp	eax, byte 0
	je	near _L13
segment	.rodata
align	4
_L14:
	db	"elif4", 0
segment	.text
	push	dword $_L14
	call	prints
	add	esp, 4
	call	println
	jmp	dword _L15
_L13:
segment	.rodata
align	4
_L16:
	db	"OK", 0
segment	.text
	push	dword $_L16
	call	prints
	add	esp, 4
	call	println
_L15:
_L12:
_L9:
_L6:
_L3:
        ;; after body 
	leave
	ret
extern	println
extern	prints
