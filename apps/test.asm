import:
	from "libstd.inc"
	putc, puts, gets
data:
	buf	string	fill(20)
code:
	call puts("What's ur name, bro? ")
	call gets(buf)
	call puts("So, hello, ")
	call puts(buf)
