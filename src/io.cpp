#include <io.h>

// Platform-dependent functions
#if (PLATFORM == PLATFORM_AVR)
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
#endif

FILE **IO::files = (FILE**)malloc(sizeof(FILE*) * IO_MAXFILES);
int IO::filesCount = 0;
struct win_t *IO::firstWindow = NULL;

void IO::displayWindow(process *p)
{
	for(int i = 0; i < pcount; i++) plist[i]->displayFlag = false;
	p->displayFlag = true;
	touchwin(p->w);
	wrefresh(p->w);
	p->pushMessage(MSG_DISPLAY, 0);
}

FILE* IO::searchFile(int fd)
{
	for(int i = 0; i < filesCount; i++)
		if(fileno(files[i]) == fd)
			return files[i];
	return NULL;
}

void IO::interrupt(process *p)
{
	if(p->regs[0] == 1) wclear(p->w);
	else if(p->regs[0] == 2) wmove(p->w, (p->regs[1] & 0xFF)-1, (p->regs[1] >> 8)-1);
	else if(p->regs[0] == 3) waddch(p->w, p->regs[1]);
	else if(p->regs[0] == 4) waddstr(p->w, (char*)(heap + p->regs[1]));
	else if(p->regs[0] == 5)
	{
		#if defined(IO_KEYBOARD_SUPPORT)
			if(p->regs[1] == 1) p->regs[0] = 1;
			else if(p->regs[1] == 2)
			{
				char c = wgetch(p->w);
				waddch(p->w, c);
				p->regs[0] = c;
			}
			else if(p->regs[1] == 3) p->regs[0] = wgetch(p->w);
			else if(p->regs[1] == 4) p->regs[0] = EOL_SYMBOL;
		#else
			p->regs[0] = 0;
		#endif
	}
	else if(p->regs[0] == 6)
	{
		int x, y;
		getmaxyx(p->w, y, x);
		p->regs[0] = (x << 8) + y;
	}
	else if(p->regs[0] == 7) curs_set(0);
	else if(p->regs[0] == 8) curs_set(1);
	else if(p->regs[0] == 9) // File I/O
	{
		if(p->regs[1] == 1) // fd fopen(str, str)
		{
			p->regs[0] = 0;
			if(filesCount == IO_MAXFILES)
				return;
			files[filesCount] = fopen((char*)(heap + p->regs[2]), (char*)(heap + p->regs[3]));
			if(files[filesCount] != NULL)
				p->regs[0] = fileno(files[filesCount++]);
		} else if(p->regs[1] == 2) // char fgetc(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			if((p->regs[0] = fgetc(f)) == EOF)
				p->regs[0] = 0x100;
		} else if(p->regs[1] == 3) // uint32_t fsize(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			if(f == NULL)
			{
				p->regs[0] = 0;
				return;
			}
			uint32_t cur = ftell(f);
			fseek(f, 0, SEEK_END);
			p->regs[0] = ftell(f);
			fseek(f, cur, SEEK_SET); 
		} else if(p->regs[1] == 4) // int fseek(fd, long, int)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			p->regs[0] = fseek(f, p->regs[3], p->regs[4]);
		} else if(p->regs[1] == 5) // int fputc(int, fd)
		{
			FILE *f = IO::searchFile(p->regs[3]);
			if((p->regs[0] = fputc(p->regs[2], f)) == EOF)
				p->regs[0] = 0x100;
		} else if(p->regs[1] == 6) // int fputs(char*, fd)
		{
			FILE *f = IO::searchFile(p->regs[3]);
			if((p->regs[0] = fputs((char*)(heap + p->regs[2]), f)) == EOF)
				p->regs[0] = 0x100;
		} else if(p->regs[1] == 7) // int fgets(char*, int, fd)
		{
			FILE *f = IO::searchFile(p->regs[4]);
			if(f == NULL) return;
			fgets((char*)(heap + p->regs[2]), p->regs[3], f);
		} else if(p->regs[1] == 8) // int fclose(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			if((p->regs[0] = fclose(f)) == EOF)
				p->regs[0] = 0x100;
		} else if(p->regs[1] == 9) // int feof(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			p->regs[0] = feof(f);
		} else if(p->regs[1] == 10) // int fflush(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			if((p->regs[0] = fflush(f)) == EOF)
				p->regs[0] = 0x100;
		} else if(p->regs[1] == 11) // int ftell(fd)
		{
			FILE *f = IO::searchFile(p->regs[2]);
			p->regs[0] = ftell(f);
		} else if(p->regs[1] == 12) // int remove(char*)
		{
			p->regs[0] = remove((char*)(heap + p->regs[2]));
		} else if(p->regs[1] == 13) // int rename(char*, char*)
		{
			p->regs[0] = rename((char*)(heap + p->regs[2]), (char*)(heap + p->regs[3]));
		}
	}
	else if(p->regs[0] == 10)
	{
		if(p->regs[1] == 1 && p->displayFlag)
		{
			wnoutrefresh(p->w);
			if(p->displayFlag) doupdate(); // refresh screen
		}
		else if(p->regs[1] == 2) // create window
		{
			int x, y;
			getmaxyx(stdscr, y, x);
			p->w = newwin(y, x, 0, 0);
			scrollok(p->w, TRUE); // enable auto-scroll
			win_t *win = (win_t*)malloc(sizeof(win_t*));
			win->owner = p;
			win->next = wins;
			wins = win;
			if(firstWindow == NULL)
			{
				displayWindow(p);
				firstWindow = win;
			}
		}
		else if(p->regs[1] == 3) displayWindow(p); // display window
		else if(p->regs[1] == 4) // get window
		{
			if(p->regs[2] == 256)
				p->w = stdscr;
			else
			{
				process *other = process::search(p->regs[2]);
				if(other != NULL) p->w = other->w;
			}
		}
		else if(p->regs[1] == 5) // scrollok
		{
			scrollok(p->w, p->regs[2]);
		}
	}
}

void IO::atexit(void *params)
{
	endwin();
}

IO::IO()
{
	for(int i = 0; i < IO_MAXFILES; i++) files[i] = (FILE*)malloc(sizeof(FILE*));
	// Init ncurses
	initscr();
	noecho();
	scrollok(stdscr, TRUE);
	// Handle ATEXIT & ATCTRLC signals
	kernel_signal(SIG_ATEXIT | SIG_ATCTRLC, &atexit, NULL);
	// Attach interrupt
	process::attachInterrupt(0x05, &interrupt);
};

IO io;
