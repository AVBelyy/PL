#include <libstd.inc>

header:
	name = test
code:
	call createwin()
	call puts("Hello! I just create new window and exit\n")
	call refresh()
	alloc r7 32
	call gets(r7)
