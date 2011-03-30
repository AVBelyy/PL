const
	NULL		0
	HEAP_SIZE	8192
data:
	#ifndef NO_HEAP
	heap	string[HEAP_SIZE]
	#endif
	process struct
		pid     int
		name    string[16]
	ends
