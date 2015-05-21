/*
** SCCS ID:	@(#)process.h	1.1	4/17/15
**
** File:	process.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Process-related declarations
*/

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

// number of user pcbs includes one for the idle process

#define	N_PCBS		(N_PROCS + 1)

// process states include a six-bit state value and two "flag" bits

#define	STATE_CODE_MASK		0x3f
#define	STATE_FLAG_MASK		0xc0

#define	STATE_FREE		0
#define	STATE_NEW		1
#define	STATE_READY		2
#define	STATE_RUNNING		3
#define	STATE_SLEEPING		4
#define	STATE_BLOCKED		5

#define	N_STATES	6

// process priorities

#define	PRIO_SYSTEM		0
#define	PRIO_USER_HIGH		1
#define	PRIO_USER_STD		2
#define	PRIO_USER_LOW		3

#define	PRIO_DEFAULT	PRIO_USER_STD

#define	N_PRIOS		4
#define	PRIO_LAST	PRIO_USER_LOW

// PID of the initial user process

#define	PID_INIT	1

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#include "clock.h"
#include "stack.h"

// ARG(n,c) - access argument #n from indicated context
//
// IF THE PARAMETER PASSING MECHANISM CHANGES, SO MUST THIS

#define	ARG(n,c)	( ((uint64_t *)((c) + 1)) [(n)] )

// RET(c) - access return value register in process context

#define RET(c)  ((c)->rax)

/*
** Types
*/

// process context structure
//
// NOTE:  the order of data members here depends on the
// register save code in isr_stubs.S!!!!

typedef struct context {
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t rbp;
	uint64_t rsp;
	uint64_t vector;
	uint64_t code;
	void    *rip;
	uint64_t cs;
	uint64_t rflags;
} context_t;

// process control block
//
// members are ordered by size

typedef struct pcb {
	// 64-bit fields
	context_t	*context;	// context save area pointer
	stack_t		*stack;		// per-process runtime stack
	uint64_t	wakeup;		// for sleeping processes

	// 16-bit fields
	int16_t		pid;		// our pid
	int16_t		ppid;		// out parent's pid

	// 8-bit fields
	uint8_t		prio;		// our priority (MLQ level)
	uint8_t		state;		// current process state
	uint8_t		quantum;	// remaining execution quantum
	uint8_t		default_quantum;	// default for this process
} pcb_t;

/*
** Globals
*/

extern pcb_t _pcbs[];		// all PCBs in the system
extern uint16_t _next_pid;	// next available PID
extern uint64_t _system_active;	// # of allocated PCBs

/*
** Prototypes
*/

/*
** _pcb_modinit()
**
** initializes all pcb-related data structures
*/

void _pcb_modinit( void );

/*
** _pcb_alloc()
**
** allocate a pcb structure
**
** returns a pointer to the pcb, or NULL on failure
*/

pcb_t *_pcb_alloc( void );

/*
** _pcb_dealloc(pcb)
**
** deallocate a pcb, putting it into the list of available pcbs
*/

void _pcb_dealloc( pcb_t *pcb );

/*
** _pcb_find(pid)
**
** locate the PCB having the specified PID
**
** returns a pointer to the PCB, or NULL if not found
*/

pcb_t *_pcb_find( int16_t pid );

/*
** _pcb_dump(pcb)
**
** dump the contents of this PCB to the console
*/

void _pcb_dump( const char *which, pcb_t *pcb );

void _context_dump( const char *which, context_t *context );

#endif

#endif
