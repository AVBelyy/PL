const:
	NO_HEAP

#include <libstd.inc>

header:
	name = test
import:
	from "libstd.def"
		ttysize, clrscr, gotoxy, putc, delay, refresh
code:
	# R5 - X coord
	# R6 - Y coord
	# R7 - direction
	# (int)R8 - Max X coord
	# (int)R9 - Max Y coord
	mov r5 1
	mov r6 1
	mov r7 1
	call clrscr()
	label loop
	call ttysize()
	mov r8 r0
	mov r9 r1
	call gotoxy( r5 r6 )
	call putc( '@' )
	call refresh()
	call gotoxy( r5 r6 )
	call putc( ' ' )
	if ( r7 == 1 ) goto RightDown
	if ( r7 == 2 ) goto RightUp
	if ( r7 == 3 ) goto LeftUp
	if ( r7 == 4 ) goto LeftDown
	label RightDown
		inc r5
		inc r6
		if ( r6 == r9 ) if ( r5 == r8 ) goto RightDownCorner
		if ( r6 == r9 ) inc r7
		if ( r5 == r8 ) add r7 3
		goto finally
		label RightDownCorner
			add r7 2
			goto finally
	label RightUp
		inc r5
		dec r6
		if ( r5 == r8 ) if ( r6 == 1 ) goto RightUpCorner
		if ( r5 == r8 ) inc r7
		if ( r6 == 1 ) dec r7
		goto finally
		label RightUpCorner
			add r7 2
			goto finally
	label LeftUp
		dec r5
		dec r6
		if ( r5 == 1 ) if ( r6 == 1 ) goto LeftUpCorner
		if ( r5 == 1 ) dec r7
		if ( r6 == 1 ) inc r7
		goto finally
		label LeftUpCorner
			sub r7 2
			goto finally
	label LeftDown
		dec r5
		inc r6
		if ( r6 == r9 ) if ( r5 == 1 ) goto LeftDownCorner
		if ( r6 == r9 ) dec r7
		if ( r5 == 1 ) sub r7 3
		goto finally
		label LeftDownCorner
			sub r7 2
			goto finally
	label finally
	call delay( 25 )
	goto loop
	label end
