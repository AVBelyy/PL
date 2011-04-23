/*
	(C) Anton Belyy, 2011
*/

#include <interpretter.h>

using namespace std;

uint8_t heap[HEAP_SIZE+1];
app_t *apps = NULL;
win_t *wins = NULL;
process *plist[MAX_PROCESS];
int_handler interrupts[MAX_INTERRUPT];
uint8_t pcount = 0;

#if (PLATFORM == PLATFORM_UNIX)
	inline void delay(uint16_t ms)
	{
		usleep((uint32_t)ms * 1000);
	}
#elif (PLATFORM == PLATFORM_WIN32)
	inline void delay(uint16_t ms)
	{
		Sleep(ms);
	}
#endif

void callproc(void *params)
{
	bool result;
	callproc_t *info = reinterpret_cast<struct callproc_t*>(params);
	process *p = info->p;
	uint16_t procid = info->procid;
	p->breakLevel = p->entryLevel+1;
	p->owner = p;
	// perform call
	p->__call(procid);
	while(result = p->exec() == true);
	p->breakLevel = 0;
}

uint16_t heap_alloc(uint16_t size)
{
	uint16_t free = 0, header = 0;
	if(!size) return HEAP_NULL;
	for(int i = 0; i < HEAP_SIZE; i++)
	{
		header = (heap[i] << 8) + heap[i+1];
		if(!header) ++free;
		else
		{
			free = 0;
			i += header + 1;
		}
		if(free == size + 2)
		{
			heap[i+1-free] = size >> 8;
			heap[i+2-free] = size & 0xFF;
			return i+3-free;
		}
	}
	return HEAP_NULL;
}

void heap_free(uint16_t ptr)
{
	if(ptr < 2 || ptr > HEAP_SIZE - 1) return;
	uint16_t size = (heap[ptr-1] << 8) + heap[ptr-2];
	if(size) memset((void*)(heap + ptr - 2), '\0', size + 2);
}

process::process(char *path)
{
	int i, len;
	errorCode = ERR_OK;
	owner = this;
	f = fopen(path, "rb");
	if(f == NULL)
	{
		errorCode = ERR_IOERROR;
		return;
	}
	len = fgetc((FILE*)f)+1;
	name = (char*)malloc(len);
	fgets(name, len, (FILE*)f);
	header.procs_cnt = fgetc((FILE*)f);
	header.lib_byte = fgetc((FILE*)f);
	header.procs = (p_procs*)malloc(header.procs_cnt*sizeof(p_procs));
	for(i = 0; i < header.procs_cnt; i++)
	{
		if(header.lib_byte >> 6 == 0b10) // if dynamic library
		{
			// read proc name, id and offset
			len = fgetc((FILE*)f)+1;
			header.procs[i].name = (char*)malloc(len);
			fgets(header.procs[i].name, len, (FILE*)f); 
 			header.procs[i].id = fgetc((FILE*)f);
			header.procs[i].offset = fgetint();
		} else { // if application or static library
			// read proc id and offset
			header.procs[i].id = fgetint();
			header.procs[i].offset = fgetint();
		}
	}
	uint16_t entryPoint = fgetint();
	header.static_size = fgetint();
	memset(regs, 0, 10 * sizeof(uint32_t));
	memset(entries, 0, MAX_ENTRIES * sizeof(p_entry));
	stackPointer = 0;
	resultFlag = true;
	lockFlag = false;
	staticPtr = heap_alloc(header.static_size);
	if(header.static_size && staticPtr == HEAP_NULL)
	{
		errorCode = ERR_NOTENOUGHMEM;
		return;
	}
	// read STATIC section into heap
	for(i = staticPtr; i < staticPtr + header.static_size; i++) heap[i] = fgetc((FILE*)f);
	breakLevel = 0;
	entries[entryLevel = 0].p = this;
	entries[0].offset = entries[0].start = entryPoint;
	// jump to program entry point
	fseek((FILE*)f, entries[0].offset, SEEK_SET);
	// set pid
	if(header.lib_byte & 0x80)
	// if library
	{
		if(header.lib_byte & 0x40)
			pid = header.lib_byte & 0x3F;
		else
			for(int i = 64; i < 128; i++)
				if(!process::search(i))
				{
					pid = i;
					break;
				}
	}
	else
	{
	// if application
		for(int i = 128; i <= 255; i++)
			if(!process::search(i))
			{
				pid = i;
				break;
			}
		// also add in 'apps' list
		app_t *app = (app_t*)malloc(sizeof(app_t*));
		app->p = this;
		app->next = apps;
		apps = app;
	}
	// by default use 'stdscr' as program's window
	displayFlag = false;
	w = stdscr;
	// insert process in process list
	plist[pcount++] = this;
	// exec signal
	sigexec(KERNEL_STARTPROCESS, (void*)this);
}
uint16_t process::fgetint() {
	FILE *file = (FILE*)owner->f;
	return (fgetc(file) << 8) + fgetc(file);
}
p_operand process::getop(bool ReturnPtr) {
	p_operand op;
	op.type = fgetc((FILE*)owner->f);
	op.value = fgetint();
	if(op.type & 0x80) // if STATIC flag
	{
		op.type ^= 0x80;
		op.value += owner->staticPtr;
	}
	if(op.type == OP_CHAR) op.value = heap[op.value];
	else if(op.type == OP_INT && !ReturnPtr) op.value = (heap[op.value] << 8) + heap[op.value+1];
	else if(op.type == OP_REG && !ReturnPtr) op.value = regs[op.value];
	else if(op.type == OP_REGPTR)
		if(ReturnPtr) op.value = regs[op.value];
		else op.value = heap[regs[op.value]];
	return op;
}
void process::__call(uint16_t addr)
{
	stackPointer = 0;
	FILE *file = (FILE*)owner->f;
	if(addr & 0xC000)
	{
		// if procedure static in library
		process *lib = process::search((addr & 0x3F00) >> 8);
		if(lib == NULL) return;
		for(int i = 0; i < lib->header.procs_cnt; i++)
			if(lib->header.procs[i].id == (addr & 0xFF))
			{
				// store current process
				entries[entryLevel].offset = ftell(file);
				entries[entryLevel++].p = owner;
				// set new process & position
				owner = lib;
				entries[entryLevel].p = lib;
				entries[entryLevel].start = entries[entryLevel].offset = lib->header.procs[i].offset;
				fseek((FILE*)owner->f, entries[entryLevel].offset, SEEK_SET);
				// lock current process
				lockFlag = true;
				break;
			}
	} else { // if procedure is local
		// search proc in proctable
		for(int i = 0; i < owner->header.procs_cnt; i++)
			if(owner->header.procs[i].id == (addr & 0xFF))
			{
				entries[entryLevel].p = owner;
				entries[entryLevel++].offset = ftell(file);
				entries[entryLevel].start = entries[entryLevel].offset = owner->header.procs[i].offset;
				fseek(file, entries[entryLevel].offset, SEEK_SET);
				break;
			}
	}
}
bool process::exec() {
	FILE *file = (FILE*)owner->f;
	uint8_t cmd = fgetc(file);
	if(feof(file)) return false;
	switch(cmd)
	{
	#ifdef __DEBUG__
	case 0x00: // NOP
	{
		if(resultFlag)
			wprintw(w, "R0 = %d\nR1 = %d\nR2 = %d\nR3 = %d\n\n", regs[0], regs[1], regs[2], regs[3]);
		break;
	}
	#endif
	// Basic arithmetic and logic operations
	case 0x01: // ADD
	case 0x02: // SUB
	case 0x05: // MUL
	case 0x06: // DIV
	case 0x07: // XOR
	case 0x08: // OR
	case 0x09: // AND
	case 0x0A: // SHL
	case 0x0B: // SHR
	case 0x16: // MOD
	{
		uint32_t index = fgetc(file), *reg = &regs[index];
		p_operand value = getop();
		if(!resultFlag) break;
		if(cmd == 0x01)			*reg += value.value;
		else if(cmd == 0x02)	*reg -= value.value;
		else if(cmd == 0x05)	*reg *= value.value;
		else if(cmd == 0x06)	*reg /= (int)value.value;
		else if(cmd == 0x07)	*reg ^= value.value;
		else if(cmd == 0x08)	*reg |= value.value;
		else if(cmd == 0x09)	*reg &= value.value;
		else if(cmd == 0x0A)	*reg <<= value.value;
		else if(cmd == 0x0B)	*reg >>= value.value;
		else if(cmd == 0x16)	*reg %= value.value;
		break;
	}
	case 0x0C: // GOTO
	{
		uint16_t offset = fgetint();
		if(!resultFlag) break;
		fseek(file, offset + entries[entryLevel].start, SEEK_SET);
		break;
	}
	case 0x0D: // INC
	case 0x0E: // DEC
	{
		uint16_t index = fgetc(file);
		if(!resultFlag) break;
		if(cmd == 0x0D)		 regs[index]++;
		else if(cmd == 0x0E) regs[index]--;
		break;
	}
	case 0x0F: // IF
	{
		char cond = fgetc(file);
		p_operand lvalue = getop(), rvalue = getop();
		if(cond == 0)		resultFlag &= (lvalue.value == rvalue.value);
		else if(cond == 1)	resultFlag &= (lvalue.value != rvalue.value);
		else if(cond == 2)	resultFlag &= (lvalue.value <= rvalue.value);
		else if(cond == 3)	resultFlag &= (lvalue.value >= rvalue.value);
		return true;
	}
	case 0x10: // MOV
	{
		p_operand addr = getop(true), value = getop();
		if(!resultFlag) break;
		if(addr.type == OP_REG) regs[addr.value] = value.value;
		else if(addr.type == OP_INT) {
			heap[addr.value] = value.value >> 8;
			heap[addr.value+1] = value.value & 0xFF;
		} else heap[addr.value] = value.value;
		break;
	}
	case 0x11: // CALL
	{
		uint16_t addr = fgetint();
		if(resultFlag)
			__call(addr);
		break;
	}
	case 0x18: // CALLD
	{
		p_operand lib_pid = getop();
		char *proc = (char*)(heap + fgetint());
		if(!resultFlag) break;
		process *lib = process::search(lib_pid.value);
		if(lib == NULL || (lib->header.lib_byte >> 6 != 0b10)) break;
		for(int i = 0; i < lib->header.procs_cnt; i++)
			if(!strcmp(lib->header.procs[i].name, proc))
			{
				// store current process
				entries[entryLevel].offset = ftell(file);
				entries[entryLevel++].p = owner;
				// set new process & position
				owner = lib;
				entries[entryLevel].p = lib;
				entries[entryLevel].start = entries[entryLevel].offset = lib->header.procs[i].offset;
				fseek((FILE*)owner->f, entries[entryLevel].offset, SEEK_SET);
				// lock current process
				lockFlag = true;
				break;
			}
		break;
	}
	case 0x12: // RET
	{
		if(!resultFlag) break;
		stackPointer = 0;
		owner = entries[--entryLevel].p;
		fseek(file, entries[entryLevel].offset, SEEK_SET);
		// unlock current process
		lockFlag = false;
		if(entryLevel == breakLevel-1) return false;
		break;
	}
	case 0x13: // PUSH
	{
		p_operand op = getop();
		if(!resultFlag) break;
		if(stackPointer < 9)
			regs[stackPointer++] = op.value;
		break;
	}
	case 0x14: // ALLOC
	{
		uint16_t index = fgetc(file);
		p_operand op = getop();
		if(resultFlag)
			regs[index] = heap_alloc(op.value);
		break;
	}
	case 0x15: // INT
	{
		p_operand num = getop();
		if(!resultFlag) break;
		stackPointer = 0;
		switch(num.value) {
		case 0x01: // INT 0x01
		{
			if(regs[0] == 1)		regs[0] = PLATFORM; // get platform
			else if(regs[0] == 2)	delay(regs[1]); // delay
			else if(regs[0] == 3)	regs[0] = rand() % 0x8000; // random
			else if(regs[0] == 4)	// signal
			{
				callproc_t sig;
				sig.p = this;
				sig.procid = regs[2];
				kernel_signal(regs[1], &callproc, (void*)&sig);
			}
			else if(regs[0] == 5)	regs[0] = (unsigned)time(NULL); // UNIX time
			else if(regs[0] == 6)	// local time
			{
				time_t unixtime;
				time(&unixtime);
				struct tm *t = localtime(&unixtime);
				char timedata[] =
				{
					t->tm_sec >> 8, t->tm_sec & 0xFF,
					t->tm_min >> 8, t->tm_min & 0xFF,
					t->tm_hour >> 8, t->tm_hour & 0xFF,
					t->tm_mday >> 8, t->tm_mday & 0xFF,
					t->tm_mon >> 8, t->tm_mon & 0xFF,
					t->tm_year >> 8, t->tm_year & 0xFF,
					t->tm_wday >> 8, t->tm_wday & 0xFF,
					t->tm_yday >> 8, t->tm_yday & 0xFF,
					t->tm_isdst >> 8, t->tm_isdst & 0xFF
				};
				memcpy((void*)(heap + regs[1]), (void*)timedata, sizeof(timedata));
			}
			break;
		}
		default:
		{
			for(int i = 0; i < MAX_INTERRUPT; i++) if(num.value == interrupts[i].id)
				interrupts[i].handler(this);
			break;
		}
		}
		break;
	}
	case 0x17: // PID
	{
		uint16_t index = fgetc(file);
		char *name = (char*)(heap + fgetint());
		if(!resultFlag) break;
		for(int i = 0; i < pcount; i++)
			if(!strcmp(plist[i]->name, name))
			{
				regs[index] = plist[i]->pid;
				break;
			}
		break;
	}
	case 0x19: // FREE
	{
		p_operand op = getop();
		if(resultFlag)
			heap_free(op.value);
		break;
	}
	}
	return resultFlag = true;
}
void process::share() {
	// if process isn't library
	if(!(header.lib_byte & 0x80)) return;
	// run init code
	while(!feof((FILE*)f)) exec();
}
process* process::search(uint16_t pid) {
	for(int i = 0; i < pcount; i++) if(plist[i]->pid == pid) return plist[i];
	return NULL;
}
void process::extcall(uint16_t procid)
{
    bool result;
    breakLevel = entryLevel+1;
    owner = this;
    // perform call
    __call(procid);
    while(result = exec() == true);
    breakLevel = 0;

}
uint8_t process::attachInterrupt(uint8_t id, void (*handler)(process*)) {
	for(int i = 0; i < MAX_INTERRUPT; i++) if(!interrupts[i].id) {
		interrupts[i].id = id;
		interrupts[i].handler = handler;
		return i;
	}
}
