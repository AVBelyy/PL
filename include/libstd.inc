const:
	NULL				0

	EOF					256
	SEEK_SET			0
	SEEK_CUR			1
	SEEK_END			2

	STDSCR				256

	KERNEL_ATEXIT		1
	KERNEL_ATCTRLC		2
	KERNEL_STARTPROCESS	4
	KERNEL_EXITPROCESS	8
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
