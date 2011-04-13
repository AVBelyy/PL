#include "stdio.h"

// Platform-dependent functions
#if (PLATFORM == PLATFORM_UNIX)
	char getch()
	{
		struct termios oldt, newt;
		int ch;
		tcgetattr( STDIN_FILENO, &oldt );
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr( STDIN_FILENO, TCSANOW, &newt );
		ch = getchar();
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
		return ch;
	}
	void clrscr()
	{
		printf("\033[2J\033[0;0f");
	}
	void gotoxy(int x, int y)
	{
		printf("\033[%d;%df", y, x); 
	}
	uint16_t ttysize()
	{
		struct winsize w;
		ioctl(0, TIOCGWINSZ, &w);
		return (w.ws_col << 8) + (w.ws_row & 0xFF);
	}
	void hidecursor()
	{
		printf("\e[?25l");
	}
	void showcursor()
	{
		printf("\e[?25h");
	}
#elif (PLATFORM == PLATFORM_WIN32)
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	void clrscr()
	{
		COORD coord = {0, 0};
		DWORD count;
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if(GetConsoleScreenBufferInfo(hStdOut, &csbi))
		{
			FillConsoleOutputCharacter(hStdOut, (TCHAR) 32, csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
			FillConsoleOutputAttribute(hStdOut, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
			SetConsoleCursorPosition(hStdOut, coord);
		}
		return;
	}
	void gotoxy(int x, int y)
	{
		COORD coord = {x-1, y-1};
		SetConsoleCursorPosition(hStdOut, coord);
	}
	uint16_t ttysize()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hStdOut, &csbi);
		return (csbi.dwSize.X << 8) + ((csbi.dwSize.Y - 1) & 0xFF);
	}
	void hidecursor()
	{
		CONSOLE_CURSOR_INFO cursor;
		cursor.bVisible = FALSE;
		SetConsoleCursorInfo(hStdOut, &cursor);
	}
	void showcursor()
	{
		CONSOLE_CURSOR_INFO cursor;
		cursor.bVisible = TRUE;
		SetConsoleCursorInfo(hStdOut, &cursor);
	}
#elif (PLATFORM == PLATFORM_AVR)
	// FOR DEBUG ONLY!!!
	FILE *stdout;
	int putchar(char a) {};
	int printf(const char* a, ...) {};
	FILE *fopen (const char* a, const char* b) {};
	unsigned int fgetc(FILE* a) {};
	char *fgets(char* a, int b, FILE* c) {};
	int fseek(FILE* a, long int b, int c) {};
	int feof(FILE* a) {};
	long int ftell(FILE* a) {};
	int puts(const char* a) {};

	void clrscr() {}
	void gotoxy(int x, int y) {}
	uint16_t ttysize() { return 0x1008; }
	void hidecursor() {}
	void showcursor() {} 
#endif

void Stdio::interrupt(process *p)
{
	if(p->regs[0] == 1) clrscr();
	else if(p->regs[0] == 2) gotoxy(p->regs[1] >> 8, p->regs[1] & 0xFF);
	else if(p->regs[0] == 3) putchar(p->regs[1]);
	else if(p->regs[0] == 4) fputs((char*)(p->mem + p->regs[1]), stdout);
	else if(p->regs[0] == 5)
	{
		#if (ENABLE_KEYBOARD_SUPPORT != 0)
		if(p->regs[1] == 1) p->regs[0] = 1;
		else if(p->regs[1] == 2)
		{
			char c = getch();
			putchar(c);
			p->regs[0] = c;
		}
		else if(p->regs[1] == 3) p->regs[0] = getch();
		else if(p->regs[1] == 4) p->regs[0] = EOL_SYMBOL;
		#else
		p->regs[0] = 0;
		#endif
	}
	else if(p->regs[0] == 6) p->regs[0] = ttysize();
	else if(p->regs[0] == 7) hidecursor();
	else if(p->regs[0] == 8) showcursor();
}

void Stdio::atexit(void *params)
{
	showcursor();
}

Stdio::Stdio()
{
	// Disable buffering
	#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_UNIX)
		setvbuf(stdout, NULL, _IONBF, 0);
	#endif
	// Set Ctrl-C and Exit handlers
	kernel_signal(KERNEL_ATEXIT | KERNEL_ATCTRLC, &atexit);
	// Attach interrupt
	process::attachInterrupt(0x05, &interrupt);
};

Stdio stdio;
