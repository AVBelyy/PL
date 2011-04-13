#include <kernel.h>
#include <interpretter.h>

struct __kernel_signal_t
{
	uint16_t type;
	void(*handler)(void *params);
};

struct __kernel_signal_t  __sighandlers[MAX_SIGHANDLERS];
uint16_t __sighandlers_cnt = 0;

void kernel_signal(uint16_t type, void(*handler)(void *params))
{
	__kernel_signal_t sig;
	sig.type = type;
	sig.handler = handler;
	__sighandlers[__sighandlers_cnt++] = sig;
}

void sigexec(uint16_t type, void *params)
{
	for(int i = 0; i < __sighandlers_cnt; i++)
		if(__sighandlers[i].type & type)
			__sighandlers[i].handler(params);
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

int main() {
	// initialize 
	for(uint8_t i = 0; i < MAX_PROCESS; i++) plist[i] = (process*)malloc(sizeof(process*));
	srand((unsigned)time(NULL));
	// set Ctrl-C handlers
	#if (PLATFORM == PLATFORM_UNIX)
		signal(SIGINT, &__atctrlc);
	#elif (PLATFORM == PLATFORM_WIN32)
		SetConsoleCtrlHandler((PHANDLER_ROUTINE)__CtrlHandler, TRUE);
	#endif
	// *** begin kernel code ***
	process l("libstd.bin");
	process p("test.bin");
	l.share();
	while(!feof((FILE*)p.f)) p.exec();
	// At exit..
	sigexec(KERNEL_ATEXIT, NULL);
	return 0;
}
