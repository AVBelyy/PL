import:
	from "libstd.inc"
		putc, puts, gets
data:
	process	struct
		pid		int
		name	string[16]
	ends
	main	process
code:
	call puts("What's ur name, bro? ")
	call gets(main.name)
	call puts("So, hello, ")
	call puts(main.name)
	call putc('\n')
