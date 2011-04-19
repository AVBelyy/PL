/*
	(C) Anton Belyy, 2011
*/

#include <kernel.h>

#ifndef INTERPRETTER_H
#define INTERPRETTER_H

#include <stdlib.h>
#include <string.h>

#if (PLATFORM == PLATFORM_AVR)
	#include <avr/pgmspace.h>
#elif (PLATFORM == PLATFORM_UNIX)
	#include <unistd.h>
#endif

// User-defined constants
#define MAX_PROCESS			16
#define MAX_INTERRUPT		16
#define MAX_ENTRIES			16
#define HEAP_SIZE			65536

// Signal types
#define KERNEL_STARTPROCESS	4

// Constants
#define OP_CONST			0
#define OP_CHAR				1
#define OP_INT				2
#define OP_REGPTR			3
#define OP_REG				4
#define OP_PROCPTR			5

#define HEAP_NULL			0

// Error codes
#define ERR_OK				0
#define ERR_NOTENOUGHMEM	1

// Structures
struct p_procs
{
	char *name;
	uint16_t id;
	uint16_t offset;
};

struct p_operand
{
	uint8_t type;
	uint32_t value;
};

class process;

struct p_entry
{
	process *p;
	uint16_t start;
	uint16_t offset;
};

struct callproc_t
{
	process *p;
	uint16_t procid;
};

struct int_handler {
	uint8_t id;
	void (*handler)(process*);
};

extern uint8_t heap[HEAP_SIZE+1];
extern process *plist[MAX_PROCESS];
extern int_handler interrupts[MAX_INTERRUPT];
extern uint8_t pcount;

void callproc(void*);
uint16_t heap_alloc(uint16_t size);
void heap_free(uint16_t ptr);

class process {
	public:
	struct {
		uint8_t procs_cnt;
		uint8_t lib_byte;
		struct p_procs *procs;
		uint16_t static_size;
	} header;
	p_entry entries[MAX_ENTRIES];
	uint8_t stackPointer, entryLevel, breakLevel;
	bool resultFlag, lockFlag;
	process *owner;
	void *f;
	uint8_t pid;
	char *name;
	uint32_t regs[10];
	uint16_t staticPtr;
	uint16_t errorCode;
	process(char*);
	uint16_t fgetint();
	p_operand getop(bool = false);
	bool exec();
	void share();
	void __call(uint16_t);
	void extcall(uint16_t);
	static process* search(uint16_t);
	static uint8_t attachInterrupt(uint8_t, void(*)(process*));
};

#endif
