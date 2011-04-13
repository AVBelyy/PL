#ifndef STDIO_H
#define STDIO_H

#include <kernel.h>
#include <interpretter.h>

//
//		stdio settings
//

#define ENABLE_KEYBOARD_SUPPORT			1

// Disable ncurses's getch()
#ifdef CURSES
	#undef getch()
#endif

//
//		Platform-dependent functions
//

#if (PLATFORM == PLATFORM_UNIX)
	#define EOL_SYMBOL		0x0A
	// In Linux: simulating getch() function from conio.h

	#include <sys/ioctl.h>
	#include <stdio.h>
	#include <termios.h>
	#include <unistd.h>

	char getch();
#elif (PLATFORM == PLATFORM_WIN32)
	#define EOL_SYMBOL		0x0D

	// In Windows: include conio.h with getch() function

	#include <stdio.h>
	#include <conio.h>
#elif (PLATFORM == PLATFORM_AVR)
	#define EOL_SYMBOL		0x0A
	#undef ENABLE_KEYBOARD_SUPPORT

	// simulating File IO in AVR
	// FOR DEBUG ONLY! 
	const int SEEK_SET = 1;
	struct FILE {
	};
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

void clrscr();
void gotoxy(int, int);
uint16_t ttysize();
void hidecursor();
void showcursor();

class Stdio {
	public:
	static void atexit(void*);
	static void interrupt(process*);
	Stdio();
};

#endif
