/*
** SCCS ID:	@(#)process.c	1.1	4/17/15
**
** File:	process.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Process-related implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include "process.h"
#include "queue.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

static queue_t _free_pcbs;	// queue of available PCBs

/*
** PUBLIC GLOBAL VARIABLES
*/

pcb_t _pcbs[ N_PCBS ];		// all the PCBs in the system
uint16_t _next_pid;		// next available PID
uint32_t _system_active;	// # of allocated PCBs

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _pcb_modinit()
**
** initializes all pcb-related data structures
*/

void _pcb_modinit( void ) {
	
	// allocate and clear the free PCB queue

	if( _queue_alloc(&_free_pcbs,1) != 1 ||
	    _free_pcbs == NULL ) {
		_kpanic( "_pcb_modinit", "free pcb queue alloc failed" );
	}

	_queue_init( _free_pcbs, NULL );
	
	// "free" all the PCBs

	for( int i = 0; i < N_PCBS; ++i ) {
		_pcb_dealloc( &_pcbs[i] );
	}
	
	// set the initial PID
	// (must be the PID of the init() process)

	_next_pid = PID_INIT;

	// no PCBS initialy in use

	_system_active = 0;

	// report that we have finished

	c_puts( " PCB" );
}

/*
** _pcb_alloc()
**
** allocate a pcb structure
**
** returns a pointer to the pcb, or NULL on failure
*/

pcb_t *_pcb_alloc( void ) {
	pcb_t *pcb;

	// pull the first available PCB off the free queue
	pcb = (pcb_t *) _queue_remove( _free_pcbs );
	if( pcb != NULL ) {
		pcb->state = STATE_NEW;
	}

	++_system_active;

	return( pcb );
}

/*
** _pcb_dealloc(pcb)
**
** deallocate a pcb, putting it into the set of available pcbs
*/

void _pcb_dealloc( pcb_t *pcb ) {
	
	// sanity check:  avoid deallocating a NULL pointer
	if( pcb == NULL ) {
		// should this be an error?
		return;
	}

	// clear the PCB (resets state to 'free')

	_memset( (void *) pcb, sizeof(pcb_t), 0 );

	// return the PCB to the free list

	_queue_insert( _free_pcbs, (void *) pcb, 0 );

	--_system_active;
}

/*
** _pcb_find(pid)
**
** locate the PCB having the specified PID
**
** returns a pointer to the PCB, or NULL if not found
*/

pcb_t *_pcb_find( int16_t pid ) {

	for( int i = 0; i < N_PROCS; ++i ) {
		if( _pcbs[i].state != STATE_FREE &&
		    _pcbs[i].pid == pid ) {
			return( &_pcbs[i] );
		}
	}

	return( NULL );
}

/*
** _pcb_dump(pcb)
**
** dump the contents of this PCB to the console
*/

void _pcb_dump( const char *which, pcb_t *pcb ) {

	c_printf( "%s @%08x: ", which, (uint32_t) pcb );
	if( pcb == NULL ) {
		c_puts( " NULL???\n" );
		return;
	}

	c_printf( " pids %d/%d state ", pcb->pid, pcb->ppid );
	switch( pcb->state ) {
		case STATE_FREE:	c_puts( "FREE" ); break;
		case STATE_NEW:	c_puts( "NEW" ); break;
		case STATE_READY:	c_puts( "READY" ); break;
		case STATE_RUNNING:	c_puts( "RUNNING" ); break;
		case STATE_SLEEPING:	c_puts( "SLEEPING" ); break;
		case STATE_BLOCKED:	c_puts( "BLOCKED" ); break;
		default:	c_printf( "? (%d)", pcb->state );
	}

	c_puts( " prio " );
	switch( pcb->prio ) {
		case PRIO_SYSTEM:		c_puts( "SYS" ); break;
		case PRIO_USER_HIGH:		c_puts( "HIGH" ); break;
		case PRIO_USER_STD:		c_puts( "STD" ); break;
		case PRIO_USER_LOW:		c_puts( "LOW" ); break;
		default:	c_printf( "? (%d)", pcb->prio );
	}

	c_printf( "\n q %d (%d) wake %08x", pcb->quantum,
		  pcb->default_quantum, pcb->wakeup );

	c_printf( " context %08x stack %08x\n",
		  (uint32_t) pcb->context, (uint32_t) pcb->stack );
}

/*
** _context_dump(context)
**
** dump the contents of this context to the console
*/

void _context_dump( const char *which, context_t *context ) {

	c_printf( "%s @%08x: ", which, (uint32_t) context );
	if( context == NULL ) {
		c_puts( " NULL???\n" );
		return;
	}

	c_printf( "\n\t ss %08x  gs %08x  fs %08x  es %08x\n",
		context->ss, context->gs, context->fs, context->es );
	c_printf( "\t ds %08x edi %08x esi %08x ebp %08x\n",
		context->ds, context->edi, context->esi, context->ebp );
	c_printf( "\tesp %08x ebx %08x edx %08x ecx %08x\n",
		context->esp, context->ebx, context->edx, context->ecx );
	c_printf( "\teax %08x vec %08x cod %08x eip %08x\n",
		context->eax, context->vector, context->code, context->eip );
	c_printf( "\t cs %08x efl %08x\n",
		context->cs, context->eflags );

}
