const:
	NO_HEAP

#include <libstd.inc>

header:
	name = test
import:
	from "libstd.def"
		clrscr, gotoxy, puts, gets, getc, getcne
code:
	call clrscr()
	call gotoxy(5 5)
	call puts("Hello from this awful language :D \n")
	call getcne()
