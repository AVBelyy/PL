/*
	(C) Anton Belyy, 2011
*/

#include <interpretter.h>

using namespace std;

process *plist[MAX_PROCESS];
int_handler interrupts[MAX_INTERRUPT];
uint8_t pcount = 0;

process::process(char *path) {
	uint8_t len, i, temp, buffer[16];
	owner = this;
	f = fopen(path, "rb");
	len = fgetc((FILE*)f)+1;
	name = (char*)malloc(len);
	fgets(name, len, (FILE*)f);
	header.procs_cnt = fgetc((FILE*)f);
	header.lib_byte = fgetc((FILE*)f);
	header.procs = (p_procs*)malloc(header.procs_cnt*4);
	for(i = 0; i < header.procs_cnt; i++) {
		header.procs[i].id = fgetint();
		header.procs[i].offset = fgetint();		
	}
	uint16_t entryPoint = fgetint();
	header.data_size = fgetint();
	// read DATA section
	memset(regs, 0, 10 * sizeof(uint32_t));
	memset(entries, 0, 10 * sizeof(p_entry));
	stackPointer = 0;
	resultFlag = true;
	lockFlag = false;
	mem = (uint8_t*)malloc(header.data_size);
	for(i = 0; i < header.data_size; i++) mem[i] = fgetc((FILE*)f);
	entries[entryLevel = 0].p = this;
	entries[0].offset = entries[0].start = entryPoint;
	fseek((FILE*)f, entries[0].offset, SEEK_SET);
	// set pid
	if(header.lib_byte & 0xC0) pid = header.lib_byte & 0x3F;
	// insert process in process list
	plist[pcount++] = this;
}
uint16_t process::fgetint() {
	return (fgetc((FILE*)owner->f) << 8) + fgetc((FILE*)owner->f);
}
p_operand process::getop(bool ReturnPtr) {
	p_operand op;
	op.type = fgetc((FILE*)owner->f);
	op.value = fgetint();
	if(op.type == OP_CHAR) op.value = mem[op.value];
	else if(op.type == OP_INT && !ReturnPtr) op.value = (mem[op.value] << 8) + mem[op.value+1];
	else if(op.type == OP_REG && !ReturnPtr) op.value = regs[op.value];
	else if(op.type == OP_REGPTR)
		if(ReturnPtr) op.value = regs[op.value];
		else op.value = mem[regs[op.value]];
	return op;
}
void process::exec() {
	uint8_t cmd = fgetc((FILE*)owner->f);
	if(feof((FILE*)owner->f)) return;
	switch(cmd) {
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
		uint32_t index = fgetc((FILE*)owner->f), *reg = &regs[index];
		p_operand value = getop();
		if(!resultFlag) break;
		if(cmd == 0x01)			*reg += value.value;
		else if(cmd == 0x02)	*reg -= value.value;
		else if(cmd == 0x05)	*reg *= value.value;
		else if(cmd == 0x06)	*reg /= (int)value.value;
		else if(cmd == 0x07)	*reg ^= value.value;
		else if(cmd == 0x08)	*reg |= value.value;
		else if(cmd == 0x09)	*reg &= value.value;
		else if(cmd == 0x0A)	*reg << value.value;
		else if(cmd == 0x0B)	*reg >> value.value;
		else if(cmd == 0x16)	*reg %= value.value;
		break;
	}
	case 0x0C: // GOTO
	{
		uint16_t offset = fgetint();
		if(!resultFlag) break;
		fseek((FILE*)owner->f, offset + entries[entryLevel].start, SEEK_SET);
		break;
	}
	case 0x0D: // INC
	case 0x0E: // DEC
	{
		uint16_t index = fgetc((FILE*)owner->f);
		if(!resultFlag) break;
		if(cmd == 0x0D)		 regs[index]++;
		else if(cmd == 0x0E) regs[index]--;
		break;
	}
	case 0x0F: // IF
	{
		char cond = fgetc((FILE*)owner->f);
		p_operand lvalue = getop(), rvalue = getop();
		if(!resultFlag) break;
		if(cond == 0)		resultFlag = (lvalue.value == rvalue.value);
		else if(cond == 1)	resultFlag = (lvalue.value != rvalue.value);
		else if(cond == 2)	resultFlag = (lvalue.value <= rvalue.value);
		else if(cond == 3)	resultFlag = (lvalue.value >= rvalue.value);
		return;
	}
	case 0x10: // MOV
	{
		p_operand addr = getop(true), value = getop();
		if(!resultFlag) break;
		if(addr.type == OP_REG) regs[addr.value] = value.value;
		else if(addr.type == OP_INT) {
			mem[addr.value] = value.value >> 8;
			mem[addr.value+1] = value.value & 0xFF;
		} else mem[addr.value] = value.value;
		break;
	}
	case 0x11: // CALL
	{
		uint16_t addr = fgetint();
		if(!resultFlag) break;
		stackPointer = 0;
		if(addr & 0xC000) { // if procedure in static library
			process *lib = process::search(addr & 0x3F00);
			if(lib == NULL) break;
			for(uint8_t i = 0; i < lib->header.procs_cnt; i++) {
				if(lib->header.procs[i].id == (addr & 0xFF)) {
					// store current process
					entries[entryLevel].offset = ftell((FILE*)owner->f);
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
			}
		} else { // if procedure is local
			// search proc in proctable
			for(uint8_t i = 0; i < owner->header.procs_cnt; i++)
				if(owner->header.procs[i].id == (addr & 0xFF)) {
					entries[entryLevel].p = owner;
					entries[entryLevel++].offset = ftell((FILE*)owner->f);
					entries[entryLevel].start = entries[entryLevel].offset = owner->header.procs[i].offset;
					fseek((FILE*)owner->f, entries[entryLevel].offset, SEEK_SET);
					break;
				}
		}
		break;
	}
	case 0x12: // RET
	{
		if(!resultFlag) break;
		owner = entries[--entryLevel].p;
		fseek((FILE*)owner->f, entries[entryLevel].offset, SEEK_SET);
		// unlock current process
		lockFlag = false;
		break;
	}
	case 0x13: // PUSH
	{
		p_operand op = getop();
		if(!resultFlag) break;
		regs[stackPointer++] = op.value;
		break;
	}
	case 0x14: // MOVF
	{
		// NOT IMPLEMENTED YET!
		// ARG1 IS A DESTIONATION
		// ARG2 IS A SOURCE IN _LOCAL_ PROGRAM MEMORY (IN LIBRARIES)
		p_operand addr = getop(true), value = getop();
		if(!resultFlag) break;
		if(addr.type == OP_REG) regs[addr.value] = value.value;
		else mem[addr.value] = value.value;
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
			if(regs[0] == 1) regs[0] = PLATFORM;
			else if(regs[0] == 2) regs[0] = 333; // 0.3.33
			break;
		}
		default:
		{
			for(uint8_t i = 0; i < MAX_INTERRUPT; i++) if(num.value == interrupts[i].id)
				interrupts[i].handler(this);
			break;
		}
		}
		break;
	}
	}
	resultFlag = true;
}
void process::share() {
	// if process isn't static library
	if(!(header.lib_byte & 0xC0)) return;
	// run init code (if it's present)
	if(entries[0].offset) while(!feof((FILE*)f)) exec();
}
process* process::search(uint16_t pid) {
	for(uint8_t i = 0; i < pcount; i++) if(plist[i]->pid == pid) return plist[i];
	return NULL;
}
uint8_t process::attachInterrupt(uint8_t id, void (*handler)(process*)) {
	if(!id || handler == NULL) return -1;
	for(uint16_t i = 0; i < MAX_INTERRUPT; i++) if(!interrupts[i].id) {
		interrupts[i].id = id;
		interrupts[i].handler = handler;
		return i;
	}
}
