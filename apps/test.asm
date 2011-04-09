const:
	NO_HEAP

#include <libstd.inc>

header:
	name = test
import:
	from "libstd.def"
		ttysize, clrscr, gotoxy, putc, delay, refresh
code:
	label loop
	call clrscr()
	call ttysize()
	mov r5 r0
	mov r6 r1
	mov r7 r5
	call gotoxy(1 1)
	label vline
	call putc('-')
	dec r7
	if (r7) goto vline
	mov r7 r5
	call gotoxy(1 r5)
	label vline2
	call putc('-')
	dec r7
	if (r7) goto vline2
	mov r7 r6
	label hline
	call gotoxy(1 r7)
	call putc('|')
	dec r7
	if (r7) goto hline
	mov r7 r6
	label hline2
	call gotoxy(r5 r7)
	call putc('|')
	dec r7
	if (r7) goto hline2
	call refresh()
	call delay(1000)
	goto loop
