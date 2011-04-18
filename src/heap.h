#ifndef HEAP_H
#define HEAP_H

#include <kernel.h>
#include <interpretter.h>

// Config

/*
	If you don't want to use or develop apps,
	that requires shared dynamic heap memory,
	you can disable it by removing USE_HEAP
*/
#define USE_HEAP

/*
	If you checked previous option (USE_HEAP),
	You must specify heap size in bytes
*/
#define HEAP_SIZE	65536

class Heap
{
	public:
	static void interrupt(process*);
	Heap();
};

#endif
