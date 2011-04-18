#include <heap.h>

/*
	Shared dynamic memomy manager
	(C) Anton Belyy, 2011
*/

#ifdef USE_HEAP

#if !defined(HEAP_SIZE) || (HEAP_SIZE == 0)
	#error "you must specify heap size"
#else

char heap[HEAP_SIZE];

void Heap::interrupt(process *p)
{
	if(p->regs[0] == 1) // malloc(size)
	{
		uint16_t i, freeMem = 0, size = p->regs[1];
		for(i = 0; i < HEAP_SIZE; i++)
		{
			if(heap[i] == '\0') ++freeMem;
			if(freeMem == size + 2)
			{
				heap[i+1-freeMem] = size >> 8;
				heap[i+2-freeMem] = size & 0xFF;
				p->regs[0] = i+3-freeMem;
				return;
			} else if(heap[i] != '\0')
			{
				freeMem = 0;
				i += (heap[i] << 8) + heap[i+1] + 1;
			}
			p->regs[0] = 0;
		}
	} else if(p->regs[0] == 2) // free(ptr)
	{
		uint16_t ptr = p->regs[1];
		if(ptr < 2 || ptr > HEAP_SIZE - 1) return;
		uint16_t size = (heap[ptr-1] >> 8) + heap[ptr-2];
		memset((void*)(heap + ptr - 2), '\0', size + 2);
	} else if(p->regs[0] == 3) // heapbyte(ptr)
	{
		uint16_t ptr = p->regs[1];
		if(ptr < 2 || ptr > HEAP_SIZE - 1) return;
		p->regs[0] = heap[ptr];
	} else if(p->regs[0] == 4) // heapint(ptr)
	{
		uint16_t ptr = p->regs[1];
		if(ptr < 2 || ptr > HEAP_SIZE - 2) return;
		p->regs[0] = (heap[ptr] << 8) + heap[ptr+1];
	} else if(p->regs[0] == 5) // heaptostatic(static_ptr heap_ptr count)
	{
		uint16_t static_ptr = p->regs[1];
		uint16_t heap_ptr = p->regs[2];
		uint16_t count = p->regs[3];
		if(heap_ptr < 2 || heap_ptr > HEAP_SIZE - 1) return;
		memcpy((void*)(p->mem + static_ptr), (void*)(heap + heap_ptr), count);
	} else if(p->regs[0] == 6) // heapfromstatic(heap_ptr static_ptr count)
	{
		uint16_t heap_ptr = p->regs[1];
		uint16_t static_ptr = p->regs[2];
		uint16_t count = p->regs[3];
		if(heap_ptr < 2 || heap_ptr > HEAP_SIZE - 1) return;
		memcpy((void*)(heap + heap_ptr), (void*)(p->mem + static_ptr), count);
	} else if(p->regs[0] == 7) // heapsz(static_ptr)
	{
		uint16_t static_ptr = p->regs[1];
		uint16_t len =strlen((char*)(p->mem + static_ptr));
		p->regs[0] = 1; p->regs[1] = len;
		interrupt(p);
		if(!p->regs[0]) return;
		strcpy((char*)(heap + p->regs[0]), (char*)(p->mem + static_ptr));
	} else if(p->regs[0] == 8) // staticsz(static_ptr heap_ptr)
	{
		uint16_t static_ptr = p->regs[1];
		uint16_t heap_ptr = p->regs[2];
		if(heap_ptr < 2 || heap_ptr > HEAP_SIZE - 1) return;
		strcpy((char*)(p->mem + static_ptr), (char*)(heap + heap_ptr));
	} else if(p->regs[0] == 9) // heapbyteset(ptr byte)
	{
		uint16_t ptr = p->regs[1];
		if(ptr < 2 || ptr > HEAP_SIZE - 1) return;
		heap[ptr] = p->regs[2] & 0xFF;
	} else if(p->regs[0] == 10) // heapintset(ptr int)
	{
		uint16_t ptr = p->regs[1];
		if(ptr < 2 || ptr > HEAP_SIZE - 2) return;
		heap[ptr] = p->regs[2] >> 8;
		heap[ptr+1] = p->regs[2] & 0xFF;
	}
}

Heap::Heap()
{
	// initialize heap
	memset(heap, '\0', HEAP_SIZE);
	process::attachInterrupt(0x02, &interrupt);
}

Heap _heap;

#endif
#endif
