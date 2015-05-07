/*
** SCCS ID:	@(#)scheduler.c	1.1	4/17/15
**
** File:	scheduler.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Scheduler/dispatcher implementation
*/

#define	__SP_KERNEL__

#include "common.h"

#include "scheduler.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

pcb_t *_current;		// the currently-running process
queue_t _ready[N_READY];	// the MLQ ready queue structure

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _sched_modinit()
**
** initialize the scheduler module
*/

void _sched_modinit( void ) {
	
	// allocate and initialize all the MLQ levels

	if( _queue_alloc(_ready,N_READY) != N_READY ) {
		_kpanic( "_sched_modinit", "ready queue alloc failed" );
	}

	for( int i = 0; i < N_READY; ++i ) {
		_queue_init( _ready[i], NULL );
	}
	
	// no current process, initially

	_current = NULL;
	
	// report that we have finished

	c_puts( " SCHED" );
}

/*
** _schedule(pcb)
**
** schedule a process for execution according to its priority
*/

void _schedule( pcb_t *pcb ) {
	
#ifdef DEBUG
	if( pcb == NULL ) {
		_kpanic( "_schedule", "NULL pcb pointer" );
	}
#endif

	// ensure the priority value for this process is legal

	if( pcb->prio >= N_PRIOS ) {
		pcb->prio = PRIO_USER_LOW;
	}
	
	// mark this process as ready

	pcb->state = STATE_READY;

	// add it to the appropriate ready queue level

// c_printf( "*** sched pid %d\n", pcb->pid );
	_queue_insert( _ready[pcb->prio], (void *)pcb, 0 );
}

/*
** _dispatch()
**
** give the CPU to a process
*/

void _dispatch( void ) {
	
	// select a process from the highest-priority
	// ready queue that is not empty

	for( int i = 0; i < N_READY; ++i ) {

		if( !_queue_empty(_ready[i]) ) {
			_current = (pcb_t *) _queue_remove( _ready[i] );
			if( _current == NULL ) {
				_kpanic( "_dispatch", "NULL from non-empty ready queue" );
			}
			_current->state = STATE_RUNNING;
			_current->quantum = _current->default_quantum;
// c_printf( "*** dispatch pid %d\n", _current->pid );
			return;
		}
	}

	// uh, oh - didn't find one

	_kpanic( "_dispatch", "no ready processes!?!?!" );
}
