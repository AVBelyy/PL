/*
	(C) Anton Belyy, 2011
*/

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define PLATFORM_AVR		1
#define PLATFORM_WIN32		2
#define PLATFORM_UNIX		3
#define PLATFORM_DOS		4

class Lib {
	public:
	Lib(void(*)());
};

#endif
