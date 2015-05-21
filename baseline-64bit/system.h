/*
** SCCS ID:	@(#)system.h	1.1	4/17/15
**
** File:	system.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Miscellaneous OS support declarations
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "types.h"
#include "process.h"
#include "stack.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// default contents of EFLAGS register

#define	DEFAULT_EFLAGS	(EFLAGS_MB1 | EFLAGS_IF)

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/*
** _create_process(entry,prio)
**
** allocate and initialize a new process' data structures (PCB, stack)
**
** returns:
**      pointer to the new PCB
*/

pcb_t *_create_process( uint32_t entry, uint8_t prio );

/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/

void _init( void );

#endif

#endif
