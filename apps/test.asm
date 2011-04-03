const:
	NO_HEAP

#include <libstd.inc>

header:
	name = test
code:
	pid r5 "dyn"
	push 0
	label loop
	call r5::foo()
	inc r0
	if (r0 != 30000) goto loop
