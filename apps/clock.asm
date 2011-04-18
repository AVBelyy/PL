#include <libstd.inc>

const:
	WN_WIDTH	25
	WN_HEIGHT	5
static:
	ttyWidth	int
	ttyHeight	int
	buffer		string[4]
	timeinfo	tm
atexit:
	call clrscr()
	call showcursor()
	ret
code:
	# R5 - loop variable
	# R6 - temp
	# R7 - temp
	# R8 - old seconds count
	# R9 - blink flag
	call signal(KERNEL_ATCTRLC atexit)
	call hidecursor()
	label mainloop
	call clrscr()
	mov r5 0
	mov r8 0
	mov r9 0
	call ttysize()
	mov &ttyWidth r0
	mov &ttyHeight r1
	sub r0 WN_WIDTH
	div r0 2
	sub r1 WN_HEIGHT
	div r1 2
	mov r6 r0
	mov r7 r1
	call gotoxy()
	label loop1
		call putc('#')
		inc r5
	if (r5 != WN_WIDTH) goto loop1
	mov r5 1
	label loop2
		inc r5
		inc r7
		call gotoxy(r6 r7)
		call putc('#')
	if (r5 != WN_HEIGHT) goto loop2
	mov r5 1
	label loop3
		call putc('#')
		inc r5
	if (r5 != WN_WIDTH) goto loop3
	mov r5 1
	add r6 WN_WIDTH
	dec r6
	label loop4
		inc r5
		dec r7
		call gotoxy(r6 r7)
		call putc('#')
	if (r5 != WN_HEIGHT) goto loop4
	mul r6 2
	sub r6 WN_WIDTH
	div r6 2
	sub r6 3
	mul r7 2
	add r7 WN_HEIGHT
	div r7 2
	label update
	call ttysize()
	if (&ttyWidth != r0) goto mainloop
	if (&ttyHeight != r1) goto mainloop
	call localtime(timeinfo)
	if (r8 == timeinfo.tm_sec) goto update
	mov r8 timeinfo.tm_sec
	xor r9 1
	call gotoxy(r6 r7)
	# print hours
	mov r0 '0'
	if (timeinfo.tm_hour <= 9) call putc()
	call itoa(buffer timeinfo.tm_hour)
	call puts()
	if (r9) push ':'	
	push ' '
	call putc()
	# print minutes
	mov r0 '0'
	if (timeinfo.tm_min <= 9) call putc()
	call itoa(buffer timeinfo.tm_min)
	call puts()
	if (r9) push ':'
	push ' '
	call putc()
	# print seconds
	mov r0 '0'
	if (timeinfo.tm_sec <= 9) call putc()
	call itoa(buffer timeinfo.tm_sec)
	call puts()
	goto update
	label end
