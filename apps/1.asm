#include <libstd.inc>

code:
	call createwin()
	label mainloop
		call popmsg()
		if (r0 == MSG_EMPTY) goto mainloop
		if (r0 == MSG_DISPLAY) goto display
	goto mainloop
	# begin msg handlers
	label display
		call puts("1")
		call refresh()
	goto mainloop
	# end msg handlers
