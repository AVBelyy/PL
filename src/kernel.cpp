#include <kernel.h>
#include <interpretter.h>

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
	for(int i = 0; i < __sighandlers_cnt; i++)
		if(__sighandlers[i].type & type)
			if(__sighandlers[i].params == NULL)
				__sighandlers[i].handler(params);
			else
				__sighandlers[i].handler(__sighandlers[i].params);
}

#if (PLATFORM == PLATFORM_UNIX)
void __atctrlc(int sig)
{
	sigexec(KERNEL_ATCTRLC, NULL);
	exit(1);
}
#elif (PLATFORM == PLATFORM_WIN32)
BOOL WINAPI __CtrlHandler(DWORD dwCtrlType)
{
	if(dwCtrlType == CTRL_C_EVENT)
	{
		sigexec(KERNEL_ATCTRLC, NULL);
		exit(1);
	}
	return TRUE;
}
#endif

int main(int argc, char *argv[]) {
	#ifdef __DEBUG__
		if(argc < 2)
		{
			printf("Usage: %s FILE [ARGS]\n", argv[0]);
			exit(1);
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
	// set Ctrl-C handlers
	#if (PLATFORM == PLATFORM_UNIX)
		signal(SIGINT, &__atctrlc);
	#elif (PLATFORM == PLATFORM_WIN32)
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)__CtrlHandler, TRUE);
	#endif
	process l("libstd.bin");
	process p1("clock.bin");
	//process p2("test.bin");
	l.share();

	app_t *app = apps;
	do
	{
		app->p->exec();
	} while(app = (app->p->lockFlag ? app : (app->next == NULL ? apps : apps->next)));
	//while(!feof((FILE*)p.f)) p.exec();
	// At exit..
	sigexec(KERNEL_ATEXIT, NULL);
	return 0;
}
