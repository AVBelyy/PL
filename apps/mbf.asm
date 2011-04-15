#include <libstd.inc>

const:
	MEM_SIZE	30000
header:
	heap = MEM_SIZE	
import:
	from "libstd.def"
		strlen, putc, getc
static:
	program string "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>."
code:
	# R6 - program length
	# R7 - current cell
	# R8 - current command
	# R9 - entry level
	call strlen(program)
	mov r6 r0
	add r6 MEM_SIZE
	mov r8 MEM_SIZE
	label exec
		if ( &r8 == '>' ) inc r7
		if ( &r8 == '<' ) dec r7
		if ( &r8 == '+' ) goto inc_cell
		if ( &r8 == '-' ) goto dec_cell
		if ( &r8 == '.' ) goto printchar
		if ( &r8 == ',' ) goto getchar
		if ( &r8 == '[' ) goto while
		if ( &r8 == ']' ) goto endwhile
		label finally
		inc r8
	if ( r6 != r8 ) goto exec
	goto end

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
		call putc()
		mov &r7 r0
	goto finally
	label while
		if ( &r7 != 0 ) goto finally
		inc r9
		label strip
			if ( r9 == 0 ) goto finally_while
			inc r8
			if ( &r8 == '[' ) inc r9
			if ( &r8 == ']' ) dec r9
		goto strip
		label finally_while
	goto finally
	label endwhile
		if ( &r7 == 0 ) goto finally
		dec r9
		label end_strip
			if ( r9 == 0 ) goto finally_endwhile
			dec r8
			if ( &r8 == '[' ) inc r9
			if ( &r8 == ']' ) dec r9
		goto end_strip
		label finally_endwhile
		dec r8
	goto finally
	# --- end subroutines ---

	label end
