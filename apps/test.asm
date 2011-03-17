import:
	include "libstd.inc"
	std::putc, std::puts, std::gets
data:
	string	biff	fill(20)
code:
	call puts( "What's ur name, bro? " )
	call gets( biff )
	call puts( "So, hello, " )
	call puts( biff )
