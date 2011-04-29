#ifndef IO_H
#define IO_H

#include <kernel.h>
#include <interpretter.h>

/*
	stdio settings
*/

#define IO_KEYBOARD_SUPPORT
#define IO_MAXFILES					32

/*
	Platform-dependent functions
*/

#if (PLATFORM == PLATFORM_UNIX) || (PLATFORM == PLATFORM_WIN32)
	#if (PLATFORM == PLATFORM_UNIX)
		#define EOL_SYMBOL		0x0A
	#else
		#define EOL_SYMBOL		0x0D
	#endif
	#include <stdio.h>
	#include <ncurses.h>
#elif (PLATFORM == PLATFORM_AVR)
	#define EOL_SYMBOL		0x0A
	#undef ENABLE_KEYBOARD_SUPPORT

	// simulating File IO in AVR
	// FOR DEBUG ONLY! 
	const int SEEK_SET = 1;
	struct FILE {};
	extern FILE *stdout;
	int putchar(char);
	int printf(const char*, ...);
	FILE *fopen (const char*, const char*);
	unsigned int fgetc(FILE*);
	char *fgets(char*, int, FILE*);
	int fseek(FILE*, long int, int);
	int feof(FILE*);
	long int ftell(FILE*);
	int fputs(const char*);
#endif

class IO
{
	public:
	static FILE **files;
	static int filesCount;
	static WINDOW *statusbar;
	static FILE *searchFile(int);
	static void displayWindow(process*);
	static void atexit(void*);
	static void interrupt(process*);
	IO();
};

#endif
