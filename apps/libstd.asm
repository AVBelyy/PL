const:
	NO_HEAP

#include <libstd.inc>

header:
	name = std
	static library = 0
delay:
	mov r1 r0
	push 2
	int 0x01
	ret
random:
	push 3
	int 0x01
	ret
strcpy:
	label loop
		mov &r0 &r1
		if (&r1==0) ret
		inc r0
		inc r1
	goto loop
strlen:
	xor r1 r1
	label loop
		if (&r0==0) ret
		inc r0
		inc r1
	goto loop
clrscr:
	push 1
	int 0x05
	ret
gotoxy:
	shl r0 8
	add r1 r0
	push 2
	int 0x05
	ret
putc:
	mov r1 r0
	push 3
	int 0x05
	ret
puts:
	mov r1 r0
	push 4
	int 0x05
	ret
itoa:
	mov r2 1
	label count
		inc r0
		mul r2 10
		if (r1>=r2) goto count
	mov r2 1
	mov &r0 0
	label loop
		dec r0
		mul r2 10
		mov r3 r1
		mod r3 r2
		mul r3 10
		div r3 r2
		add r3 48
		mov &r0 r3
		if (r1>=r2) goto loop
	ret
atoi:
	xor r3 r3
	mov r2 1
	label count
		inc r0
		if (&r0) goto count
	dec r0
	label loop
		mov r1 &r0
		sub r1 48
		mul r2 10
		mul r1 r2
		add r3 r1
		dec r0
		if (&r0) goto loop
	div r3 10
	push r3
	ret
getc:
	push 5
	push 2
	int 0x05
	ret
getcne:
	push 5
	push 3
	int 0x05
	ret
gets:
	mov r2 r0
	push 5
	push 4
	int 0x05
	mov r3 r0
	label read
		call getc()
		if (r0==r3) goto break
		mov &r2 r0
		inc r2
	goto read
	label break
	mov &r2 0
	push 1
	int 0x01
	if (r0 == 1) ret
	if (r0 == 3) ret
	call putc( '\n' )
	ret
ttysize:
	push 6
	int 0x05
	mov r1 r0
	shr r0 8
	and r1 0xFF
	ret
malloc:
	#	R0		- size in bytes
	#	(int)R1	- loop counter
	#	(int)R2	- max block size	
	if (r0>=256) goto fail
	xor r1 r1
	xor r2 r2
	label loop
	if (&r1==0) goto then
	xor r2 r2
	add r1 &r1
	goto endif
	label then
	inc r2
	label endif
	inc r1
	if (r0!=r2) goto continue
	sub r1 r2
	mov &r1 r0
	inc r1
	push r1
	ret
	label continue
	if (r1!=HEAP_SIZE) goto loop
	label fail
	push NULL
	ret
free:
	#	R0		- ptr
	#	(int)R1	- loop counter
	if(r0==NULL) ret
	dec r0
	mov r1 &r0
	if (r1==0) ret
	label loop
	inc r0
	dec r1
	mov &r0 0
	if (r1) goto loop
	ret
realloc:
	#	R0		- ptr
	#	R1		- new size
	mov r2 r1
	call free()
	call malloc(r2)
	ret
export:
	delay, random
	clrscr, gotoxy, ttysize
	putc, puts
	getc, getcne, gets
	itoa, atoi
	strcpy, strlen
	malloc, realloc, free
