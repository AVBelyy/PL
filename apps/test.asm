#include <libstd.inc>

import:
	from "libstd.def"
		putc, puts, gets
		atoi, itoa
data:
	buffer	string[100]
	self	process
code:
	mov		self.pid	4512
	mov		r0			self.pid
	call itoa(buffer self.pid)
	call puts("NUMBER = ")
	call puts(buffer)
	call putc('\n')

