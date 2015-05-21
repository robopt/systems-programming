/*
** SCCS ID:	@(#)stack.h	1.1	4/17/15
**
** File:	stack.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Stack-related declarations
*/

#ifndef _STACK_H_
#define _STACK_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// size of each stack (in longwords)

#define	STACK_LWORDS	1024

// number of stacks to create includes one for the idle process

#define	N_STACKS	(N_PROCS)

/*
** Types
*/

// stack structure

typedef uint32_t stack_t[STACK_LWORDS];

/*
** Globals
*/

extern stack_t _system_stack;		// separate stack for the OS itself
extern uint32_t *_system_esp;		// OS %ESP value

/*
** Prototypes
*/

/*
** _stack_modinit()
**
** initializes all stack-related data structures
*/

void _stack_modinit( void );

/*
** _stack_alloc()
**
** allocate a stack structure
**
** returns a pointer to the stack, or NULL on failure
*/

stack_t *_stack_alloc( void );

/*
** _stack_dealloc(stack)
**
** deallocate a stack, putting it into the list of available stacks
*/

void _stack_dealloc( stack_t *stack );

#endif

#endif
