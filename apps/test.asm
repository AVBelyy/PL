const:
	NO_HEAP

#include <libstd.inc>

header:
	name = test
data:
	buf		string[2]
import:
	from "libstd.def"
		gets, puts, malloc
	
code:
	pid r3 "dyn"
	call r3::foo(1 2 3)
	nop
	push 0
	label loop
	inc r0
	if (r0==32000) goto end
	goto loop
	label end
	/call gets()
