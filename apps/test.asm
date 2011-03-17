import:
	include "libstd.inc"
	std::putc, std::puts, std::gets
data:
	string	buf	fill(20)
code:
	call puts("What's ur name, bro? ")
	call gets(buf)
	call puts("So, hello, ")
	call puts(buf)
