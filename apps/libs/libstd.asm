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
signal:
	mov r2 r1
	mov r1 r0
	push 4
	int 0x01
	ret
time:
	push 5
	int 0x01
	ret
localtime:
	mov r1 r0
	push 6
	int 0x01
	ret
pushmsg:
	mov r3 r2
	mov r2 r1
	mov r1 r0
	push 7
	int 0x01
	ret
popmsg:
	push 8
	int 0x01
	ret
strcpy:
	label loop
		mov *r0 *r1
		if (*r1==0) ret
		inc r0
		inc r1
	goto loop
strlen:
	xor r1 r1
	label loop
		if (*r0==0) goto return
		inc r0
		inc r1
	goto loop
	label return
	mov r0 r1
	ret
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
	mov *r0 0
	label loop
		dec r0
		mul r2 10
		mov r3 r1
		mod r3 r2
		mul r3 10
		div r3 r2
		add r3 48
		mov *r0 r3
		if (r1>=r2) goto loop
	ret
atoi:
	xor r3 r3
	mov r2 1
	label count
		inc r0
		if (*r0) goto count
	dec r0
	label loop
		mov r1 *r0
		sub r1 48
		mul r2 10
		mul r1 r2
		add r3 r1
		dec r0
		if (*r0) goto loop
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
		ret
		mov *r2 r0
		inc r2
	goto read
	label break
	mov *r2 0
	push 1
	int 0x01
	if (r0 == 1) ret
	if (r0 == 3) ret
	call putc('\n')
	ret
ttysize:
	push 6
	int 0x05
	mov r1 r0
	shr r0 8
	and r1 0xFF
	ret
hidecursor:
	push 7
	int 0x05
	ret
showcursor:
	push 8
	int 0x05
	ret
fopen:
	mov r3 r1
	mov r2 r0
	push 9
	push 1
	int 0x05
	ret
fgetc:
	mov r2 r0
	push 9
	push 2
	int 0x05
	ret
fsize:
	mov r2 r0
	push 9
	push 3
	int 0x05
	ret
fseek:
	mov r4 r2
	mov r3 r1
	mov r2 r0
	push 9
	push 4
	int 0x05
	ret
fputc:
	mov r3 r1
	mov r2 r0
	push 9
	push 5
	int 0x05
	ret
fputs:
	mov r3 r1
	mov r2 r0
	push 9
	push 6
	int 0x05
	ret
fgets:
	mov r4 r2
	mov r3 r1
	mov r2 r0
	push 9
	push 7
	int 0x05
	ret
fclose:
	mov r2 r0
	push 9
	push 8
	int 0x05
	ret
feof:
	mov r2 r0
	push 9
	push 9
	int 0x05
	ret
fflush:
	mov r2 r0
	push 9
	push 10
	int 0x05
	ret
ftell:
	mov r2 r0
	push 9
	push 11
	int 0x05
	ret
rewind:
	mov r4 r0
	push 9
	push 4
	push 0
	push r4
	push SEEK_SET
	int 0x05
	ret
remove:
	mov r2 r0
	push 9
	push 12
	int 0x05
	ret
rename:
	mov r3 r1
	mov r2 r0
	push 9
	push 13
	int 0x05
	ret
refresh:
	push 10
	push 1
	int 0x05
	ret
createwin:
	push 10
	push 2
	int 0x05
	ret
displaywin:
	push 10
	push 3
	int 0x05
	ret
setwin:
	mov r2 r0
	push 10
	push 4
	int 0x05
	ret
scroll:
	push 10
	push 5
	push 1
	int 0x05
	ret
noscroll:
	push 10
	push 5
	push 0
	int 0x05
	ret
export:
	delay, random, signal, time, localtime, pushmsg, popmsg
	hidecursor, showcursor
	clrscr, gotoxy, ttysize
	fopen, fgetc, fsize, fseek, fputc, fputs, fgets
	fclose, feof, fflush, ftell, rewind, rename, remove
	createwin, displaywin, setwin, refresh, scroll, noscroll
	putc, puts
	getc, getcne, gets
	itoa, atoi
	strcpy, strlen
