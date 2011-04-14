import:
	from "libstd.def"
		strlen, putc, getc
data:
	mem		string[30000]
	program string "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++.+++++++++++++++++++++++++++++.+++++++..+++.-------------------------------------------------------------------------------.+++++++++++++++++++++++++++++++++++++++++++++++++++++++.++++++++++++++++++++++++.+++.------.--------.-------------------------------------------------------------------.-----------------------."
code:
	goto start

	# --- begin subroutines ---
	label printchar
		call putc( &r7 )
	goto finally
	label inc_cell
		mov r0 &r7
		inc r0
		mov &r7 r0
	goto finally
	label dec_cell
		mov r0 &r7
		dec r0
		mov &r7 r0
	goto finally
	label getchar
		call putc()
		mov &r7 r0
	goto finally
	# --- end subroutines ---

	label start
	# R5 - loop variable
	# R6 - program length
	# R7 - current cell
	# R8 - cur command ptr
	call strlen(program)
	mov r5 0
	mov r6 r0
	mov r7 0
	label exec
		mov r8 30000
		add r8 r5
		if ( &r8 == '>' ) inc r7
		if ( &r8 == '<' ) dec r7
		if ( &r8 == '+' ) goto inc_cell
		if ( &r8 == '-' ) goto dec_cell
		if ( &r8 == '.' ) goto printchar
		if ( &r8 == ',' ) goto getchar
		label finally
		inc r5
	if ( r5 != r6 ) goto exec
