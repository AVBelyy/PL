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
#else
	#include <unistd.h>
#endif

// User-defined constants
#define MAX_PROCESS			16
#define MAX_INTERRUPT		16

// Signal types
#define KERNEL_NEWPROCESS	4

// Constants
#define OP_CONST			0
#define OP_CHAR				1
#define OP_INT				2
#define OP_REGPTR			3
#define OP_REG				4
#define OP_PROCPTR			5

// Structures
struct p_procs {
	char *name;
	uint16_t id;
	uint16_t offset;
};

struct p_operand {
	uint8_t type;
	uint32_t value;
};

class process;

struct p_entry {
	process *p;
	uint16_t start;
	uint16_t offset;
};

struct int_handler {
	uint8_t id;
	void (*handler)(process*);
};

extern process *plist[MAX_PROCESS];
extern int_handler interrupts[MAX_INTERRUPT];
extern uint8_t pcount;

class process {
	public:
	struct {
		uint8_t procs_cnt;
		uint8_t lib_byte;
		struct p_procs *procs;
		uint16_t data_size;
	} header;
	p_entry entries[10];
	uint8_t stackPointer, entryLevel, breakLevel;
	bool resultFlag, lockFlag;
	process *owner;
	void *f;
	uint8_t pid;
	char *name;
	uint32_t regs[10];
	uint8_t *mem;
	process(char*);
	uint16_t fgetint();
	p_operand getop(bool = false);
	bool exec();
	void share();
	void __call(uint16_t);
	static process* search(uint16_t);
	static uint8_t attachInterrupt(uint8_t, void(*)(process*));
};

#endif
