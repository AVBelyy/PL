const:
	NULL				0

	#IO Constants
	EOF					256
	SEEK_SET			0
	SEEK_CUR			1
	SEEK_END			2
	WN_STDSCR			256
	WN_STATUSBAR		257
	WN_CURRENT			258				

	# Signals
	SIG_ATEXIT			1
	SIG_ATCTRLC			2
	SIG_STARTPROCESS	4

	# Messages
	MSG_EMPTY			0
	MSG_DISPLAY			1
static:
	process	struct
		pid     	int
		name    	string[16]
	ends
	tm	struct
		tm_sec		int
		tm_min		int
		tm_hour		int
		tm_mday		int
		tm_mon		int
		tm_year		int
		tm_wday		int
		tm_yday		int
		tm_isdst	int
	ends
