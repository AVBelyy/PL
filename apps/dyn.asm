const:
	NO_HEAP

#include <libstd.inc>

header:
	name = dyn
	dynamic library
foo:
	add r0 1234
	ret
export:
	foo
