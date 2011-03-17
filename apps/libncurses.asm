header:
	name = ncurses
	static library = 0x11
initscr:
	push 1
	int 0x11
	ret
endwin:
	push 2
	int 0x11
	ret
export:
	initscr, endwin
