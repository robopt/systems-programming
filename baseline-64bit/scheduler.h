/*
** SCCS ID:	@(#)scheduler.h	1.1	4/17/15
**
** File:	scheduler.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Scheduler/dispatcher module declarations
*/

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#include "process.h"
#include "queue.h"

// number of ready queues - one per priority level

#define	N_READY		N_PRIOS

/*
** Types
*/

/*
** Globals
*/

extern pcb_t *_current;		// the currently-running process
extern queue_t _ready[];	// the MLQ ready queue structure

/*
** Prototypes
*/

/*
** _sched_modinit()
**
** initialize the scheduler module
*/

void _sched_modinit( void );

/*
** _schedule(pcb)
**
** schedule a process for execution according to its priority
*/

void _schedule( pcb_t *pcb );

/*
** _dispatch()
**
** give the CPU to a process
*/

void _dispatch( void );

#endif

#endif
