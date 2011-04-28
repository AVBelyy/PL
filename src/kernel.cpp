#include <kernel.h>
#include <interpretter.h>
#include <io.h>

struct __kernel_signal_t
{
	uint16_t type;
	void(*handler)(void *params);
	void *params;
};

struct __kernel_signal_t  __sighandlers[MAX_SIGHANDLERS];
uint16_t __sighandlers_cnt = 0;

void kernel_signal(uint16_t type, void(*handler)(void *params), void *params)
{
	__kernel_signal_t sig;
	sig.type = type;
	sig.handler = handler;
	sig.params = params;
	__sighandlers[__sighandlers_cnt++] = sig;
}

void sigexec(uint16_t type, void *params)
{
	int i = __sighandlers_cnt;
	do
	{
		--i;
		if(__sighandlers[i].type & type)
			if(__sighandlers[i].params == NULL)
				__sighandlers[i].handler(params);
			else
				__sighandlers[i].handler(__sighandlers[i].params);
	} while(i);
}

void __winswitch(int sig)
{
	static win_t *win = IO::firstWindow;
	if(win == NULL) return; // if there's no windows
	win = (win->next == NULL ? wins : win->next);
	IO::displayWindow(win->owner);
}
#if (PLATFORM == PLATFORM_UNIX)
void __atctrlc(int sig)
{
	sigexec(SIG_ATCTRLC, NULL);
	exit(1);
}
#elif (PLATFORM == PLATFORM_WIN32)
BOOL WINAPI __CtrlHandler(DWORD dwCtrlType)
{
	if(dwCtrlType == CTRL_C_EVENT)
	{
		sigexec(SIG_ATCTRLC, NULL);
		exit(1);
	}
	// TODO: Handle CTRL-\ as window switching combination
	return TRUE;
}
#endif

int main(int argc, char *argv[]) {
	#ifdef __DEBUG__
		if(argc < 2)
		{
			sigexec(SIG_ATEXIT, NULL); // reset normal terminal state
			printf("Usage: %s FILE [ARGS]\n", argv[0]);
			return 1;
		}
		char *filename = argv[1];
		if(argc == 2)
			char *initArgs = "";
		else
			char *initArgs = argv[2];
	#else
		char *filename = "clock.bin";
		char *initArgs = "";
	#endif
	// initialize
	for(int i = 0; i < MAX_PROCESS; i++) plist[i] = (process*)malloc(sizeof(process*));
	srand((unsigned)time(NULL));
	// set Ctrl-C & window switch handlers
	#if (PLATFORM == PLATFORM_UNIX)
		signal(SIGINT, &__atctrlc);
		signal(SIGQUIT, &__winswitch);
	#elif (PLATFORM == PLATFORM_WIN32)
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)__CtrlHandler, TRUE);
	#endif
	// set window switch handler
	process l("libstd.bin");
	process p1("1.bin");
	process p2("2.bin");
	//process p3("bf.bin");
	l.share();

	app_t *app = apps;
	do
	{
		app->p->exec();
	} while(app = (app->p->lockFlag ? app : (app->next == NULL ? apps : app->next)));
	// At exit..
	sigexec(SIG_ATEXIT, NULL);
	return 0;
}
