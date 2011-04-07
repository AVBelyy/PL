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
		system("clear");
	}
	void gotoxy(int x, int y)
	{
		printf("%c[%d;%df", 0x1B, y, x); 
	}
#elif (PLATFORM == PLATFORM_WIN32)
	void clrscr()
	{
		system("cls");
	}
	void gotoxy(int x, int y)
	{
		COORD coord;
		coord.X = x;
		coord.Y = y;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
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
};
Stdio::Stdio()
{
	process::attachInterrupt(0x05, &interrupt);
};

Stdio stdio;
