#include <libstd.inc>

const:
	MEM_SIZE	40000
header:
	heap = MEM_SIZE	
import:
	from "libstd.def"
		malloc
		strlen, putc, getc
		fopen, fgetc, fsize, fseek
code:
	# R5 - current command
	# R6 - file descriptor
	# R7 - current cell
	# R8 - current command number
	# R9 - entry level
	call fopen("program.bf" "rb")
	mov r6 r0
	label exec
		call fgetc( r6 )
		if ( r0 == EOF ) goto end
		if ( r0 == '>' ) inc r7
		if ( r0 == '<' ) dec r7
		if ( r0 == '+' ) goto inc_cell 
		if ( r0 == '-' ) goto dec_cell
		if ( r0 == '.' ) goto printchar
		if ( r0 == ',' ) goto getchar
		if ( r0 == '[' ) goto while
		if ( r0 == ']' ) goto endwhile
		label finally
		inc r8
		call fseek(r6 r8 SEEK_SET)
	goto exec

	# --- begin subroutines ---
	label printchar
		call putc( &r7 )
	goto finally
	label inc_cell
		mov r0 &r7
		inc r0
		mod r0 0xFF
		mov &r7 r0
	goto finally
	label dec_cell
		mov r0 &r7
		dec r0
		mod r0 0xFF
		mov &r7 r0
	goto finally
	label getchar
		call getc()
		mov &r7 r0
	goto finally
	label while
		if ( &r7 != 0 ) goto finally
		inc r9
		label strip
			if ( r9 == 0 ) goto finally_while
			inc r8
			call fgetc( r6 )
			if ( r0 == '[' ) inc r9
			if ( r0 == ']' ) dec r9
		goto strip
		label finally_while
	goto finally
	label endwhile
		if ( &r7 == 0 ) goto finally
		dec r9
		label end_strip
			if ( r9 == 0 ) goto finally_endwhile
			dec r8
			call fseek( r6 r8 SEEK_SET )
			call fgetc( r6 )
			if ( r0 == '[' ) inc r9
			if ( r0 == ']' ) dec r9
		goto end_strip
		label finally_endwhile
		dec r8
	goto finally
	# --- end subroutines ---

	label end