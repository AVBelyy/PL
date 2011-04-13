/*
	(C) Anton Belyy, 2011
*/

#ifndef KERNEL_H
#define KERNEL_H

// Platforms
#define PLATFORM_AVR		1
#define PLATFORM_WIN32		2
#define PLATFORM_UNIX		3
#define PLATFORM_DOS		4

// Config
#define	MAX_SIGHANDLERS		32

// Signal types
#define KERNEL_ATEXIT		1
#define KERNEL_ATCTRLC		2

#include <stdint.h>
#include <time.h>

#if (PLATFORM == PLATFORM_UNIX)
	#include <signal.h>
#elif (PLATFORM == PLATFORM_WIN32)
	#include <windows.h>
#endif

void kernel_signal(uint16_t, void(*)(void*), void*);
void sigexec(uint16_t, void*);

#endif
