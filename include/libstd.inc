const:
	NULL				0

	EOF					256
	SEEK_SET			0
	SEEK_CUR			1
	SEEK_END			2

	KERNEL_ATEXIT		1
	KERNEL_ATCTRLC		2
	KERNEL_NEWPROCESS	4
static:
	process struct
		pid     int
		name    string[16]
	ends
