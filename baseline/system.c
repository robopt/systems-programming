/*
** SCCS ID:	@(#)system.c	1.1	4/17/15
**
** File:	system.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Miscellaneous OS support implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include "system.h"
#include "clock.h"
#include "process.h"
#include "bootstrap.h"
#include "syscall.h"
#include "sio.h"
#include "net.h"
#include "ata.h"
#include "pci.h"
#include "scheduler.h"

// need address of the initial user process
#include "user.h"

// need the __default_exit__() prototype
#include "ulib.h"

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

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _create_process(entry,prio)
**
** allocate and initialize a new process' data structures (PCB, stack)
**
** returns:
**      pointer to the new PCB
*/

pcb_t *_create_process( void *entry, uint8_t prio ) {
	pcb_t *new;
	uint64_t *ptr;

	// allocate the new structures

	new = _pcb_alloc();
	if( new == NULL ) {
		return( NULL );
	}

	// clear all the fields in the PCB

	_memset( (void *) new, sizeof(pcb_t), 0 );

	// allocate the runtime stack

	new->stack = _stack_alloc();
	if( new->stack == NULL ) {
		_pcb_dealloc( new );
		return( NULL );
	}

	/*
	** We need to set up the initial stack contents for the new
	** process.  The low end of the initial stack must look like this:
	**
	**      esp ->  ?     <- context save area
	**              ...   <- context save area
	**              ?     <- context save area
	**              exit  <- return address for faked call to main()
	**              0     <- last word in stack
	*/

	// first, create a pointer to the longword after the stack

	ptr = (uint64_t *) (new->stack + 1);

	// save the buffering 0 at the end

	*--ptr = 0;

	// fake a return address so that if the user function returns
	// without calling exit(), we return "into" a function which
	// calls exit()

	*--ptr = (uint64_t) __default_exit__;

	// locate the context save area

	new->context = ((context_t *) ptr) - 1;

	// fill in the non-zero entries in the context save area

	new->context->rip    = entry;
	new->context->rflags = DEFAULT_RFLAGS;

	// fill in the remaining important fields

	new->prio = prio;
	new->pid  = _next_pid++;
	new->default_quantum = QUANTUM_DEFAULT;
	new->state = STATE_READY;

	// all done - return the new PCB

	return( new );
}

/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/

void _init( void ) {
	pcb_t *pcb;

	/*
	** BOILERPLATE CODE - taken from basic framework
	**
	** Initialize interrupt stuff.
	*/

	__init_interrupts();	// IDT and PIC initialization

	/*
	** Console I/O system.
	*/

	c_io_init();
	c_setscroll( 0, 7, 99, 99 );
	c_puts_at( 0, 6, "================================================================================" );

	/*
	** 20145-SPECIFIC CODE STARTS HERE
	*/

	/*
	** Initialize various OS modules
	**
	** Note:  the clock, SIO, and syscall modules also install
	** their ISRs.
	*/

	c_puts( "Module init: " );

	_queue_modinit();		// must be first
	_pcb_modinit();
	_stack_modinit();
	_sched_modinit();
	_sio_modinit();
	_sys_modinit();
	_clock_modinit();
    _pci_modinit();
    _net_modinit();
    _ata_modinit();
	_kpanic( "_init", "_net_modinit finished" );

	c_puts( "\n" );

	/*
	** Create the initial system ESP
	**
	** This will be the address of the next-to-last
	** longword in the system stack.
	*/

	_system_rsp = ((uint64_t *) ( (&_system_stack) + 1)) - 2;

	/*
	** Create the initial process
	**
	** Code mostly stolen from _sys_spawnp(); if that routine
	** changes, SO MUST THIS!!!
	*/

	pcb = _create_process( (void*)(unsigned long)init, PRIO_SYSTEM );
	if( pcb == NULL ) {
		_kpanic( "_init", "init() creation failed" );
	}

	_pcb_dump( "init() pcb", pcb );
	_context_dump( "init() context", pcb->context );

	_schedule( pcb );

	/*
	** Next, create the idle process
	*/

	pcb = _create_process( (void*)(unsigned long)idle, PRIO_USER_LOW );
	if( pcb == NULL ) {
		_kpanic( "_init", "idle() creation failed" );
	}

	_schedule( pcb );

	/*
	** Turn on the SIO receiver (the transmitter will be turned
	** on/off as characters are being sent)
	*/

	_sio_enable( SIO_RX );

	/*
	** Send the first process off to play
	*/

	_dispatch();

	/*
	** END OF 20145-SPECIFIC CODE
	**
	** Finally, report that we're all done.
	*/

	c_puts( "System initialization complete.\n" );

}
