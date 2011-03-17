header:
	name = std
	static library = 0
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
putc:
	mov r1 r0
	push 3
	int 0x05
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
//	call putc( '\r' )
	call putc( '\n' )
	ret
export:
	putc, puts
	getc, gets
	itoa, atoi
	strcpy, strlen
