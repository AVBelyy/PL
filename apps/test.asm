const:
	NO_HEAP

#include <libstd.inc>

data:
	buf		string[2]
import:
	from "libstd.def"
		gets, puts, malloc
	
code:
	push 0
	label loop
	inc r0
	if (r0==32000) goto end
	goto loop
	label end
	/call gets()
