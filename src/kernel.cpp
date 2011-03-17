#include <kernel.h>
#include <interpretter.h>

Lib::Lib(void (*main)()) {
	main();
}

int main() {
	// initialize
	for(uint8_t i = 0; i < MAX_PROCESS; i++) plist[i] = (process*)malloc(sizeof(process*));
	process l("libstd.bin");
	process p("test.bin");
	l.share();
	while(!feof((FILE*)p.f)) p.exec();
	return 0;
}
