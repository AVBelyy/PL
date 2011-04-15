const:
	NULL		0

	KERNEL_ATEXIT		1
	KERNEL_ATCTRLC		2
	KERNEL_NEWPROCESS	4
static:
	process struct
		pid     int
		name    string[16]
	ends
