/*
** SCCS ID:	@(#)clock.c	1.1	4/17/15
**
** File:	clock.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Clock module implementation
*/

#define	__SP_KERNEL__


#include <x86arch.h>
#include "startup.h"

#include "clock.h"
#include "process.h"
#include "queue.h"
#include "scheduler.h"
#include "sio.h"
#include "syscall.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// pinwheel control variables

static uint32_t _pinwheel;	// pinwheel counter
static uint32_t _pindex;	// index into pinwheel string

/*
** PUBLIC GLOBAL VARIABLES
*/

uint32_t _system_time;		// the current system time

/*
** PRIVATE FUNCTIONS
*/


/*
** _clock_isr(vector,code)
**
** Interrupt handler for the clock module.  Spins the pinwheel,
** wakes up sleeping processes, and handles quantum expiration
** for the current process.
*/

static void _clock_isr( int vector, int code ) {
	pcb_t *pcb;

	// spin the pinwheel

	++_pinwheel;
	if( _pinwheel == (CLOCK_FREQUENCY / 10) ) {
		_pinwheel = 0;
		++_pindex;
		c_putchar_at( 79, 0, "|/-\\"[ _pindex & 3 ] );
	}

	// increment the system time

	++_system_time;

	/*
	** wake up any sleeper whose time has come
	**
	** we give awakened processes preference over the
	** current process (when it is scheduled again)
	*/

	while( !_queue_empty(_sleeping) &&
	       (uint32_t) _queue_kpeek(_sleeping) <= _system_time ) {

		// time to wake up!  remove it from the queue
		pcb = (pcb_t *) _queue_remove( _sleeping );
		if( pcb == NULL ) {
#ifdef DEBUG
			_kpanic( "_clock_isr", "NULL from sleep queue remove" );
#else
			c_puts( "*** _clock_isr: NULL from sleep queue\n" );
			break;
#endif
		}

		// and schedule it for dispatch
		_schedule( pcb );
	}
	
	// check the current process to see if it needs to be scheduled

	// sanity check!

	_current->quantum -= 1;
	if( _current->quantum < 1 ) {
		_schedule( _current );
		_dispatch();
	}

#ifdef DUMP_QUEUES
	// Approximately every 10 seconds, dump the queues, and
	// print the contents of the SIO buffers.

	if( (_system_time % SECONDS_TO_TICKS(10)) == 0 ) {
		c_printf( "Queue contents @%08x\n", _system_time );
		_queue_dump( "ready[0]", _ready[0] );
		_queue_dump( "ready[1]", _ready[1] );
		_queue_dump( "ready[2]", _ready[2] );
		_queue_dump( "ready[3]", _ready[3] );
		_queue_dump( "sleep", _sleeping );
		_sio_dump();
	}
#endif

	// tell the PIC we're done

	__outb( PIC_MASTER_CMD_PORT, PIC_EOI );

}

/*
** PUBLIC FUNCTIONS
*/

/*
** _clock_modinit()
**
** initialize the clock module
*/

void _clock_modinit( void ) {
	uint32_t divisor;

	// start the pinwheel

	_pinwheel = _pindex = 0;

	// return to the epoch

	_system_time = 0;

	// set the clock to tick at CLOCK_FREQUENCY Hz.

	divisor = TIMER_FREQUENCY / CLOCK_FREQUENCY;
	__outb( TIMER_CONTROL_PORT, TIMER_0_LOAD | TIMER_0_SQUARE );
        __outb( TIMER_0_PORT, divisor & 0xff );        // LSB of divisor
        __outb( TIMER_0_PORT, (divisor >> 8) & 0xff ); // MSB of divisor

	// register the ISR

	__install_isr( INT_VEC_TIMER, _clock_isr );

        // announce that we have initialized the clock module

        c_puts( " CLOCK" );
}
