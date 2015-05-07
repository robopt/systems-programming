/*
** SCCS ID:	@(#)syscall.c	1.1	4/17/15
**
** File:	syscall.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	System call module implementation
*/

#define	__SP_KERNEL__

#include "common.h"

#include "syscall.h"

#include "process.h"
#include "stack.h"
#include "queue.h"
#include "scheduler.h"
#include "sio.h"

#include "support.h"
#include "startup.h"
#include <x86arch.h>

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// system call jump table
//
// initialized by _sys_modinit() to ensure that
// code::function mappings are correct

static void (*_syscalls[ N_SYSCALLS ])( pcb_t * );

/*
** PUBLIC GLOBAL VARIABLES
*/

queue_t _sleeping;	// sleeping processes

/*
** PRIVATE FUNCTIONS
*/

/*
** _sys_isr(vector,code)
**
** Common handler for the system call module.  Selects
** the correct second-level routine to invoke based on
** the contents of EAX.
**
** The second-level routine is invoked with a pointer to
** the PCB for the process.  It is the responsibility of
** that routine to assign all return values for the call.
*/

static void _sys_isr( int vector, int code ) {
	uint32_t which= _current->context->eax;

	// verify that we were given a legal code

	if( which >= N_SYSCALLS ) {

		// nope - report it...

		c_printf( "*** _sys_isr, PID %d syscall %d\n",
			_current->pid, which );

		// ...and force it to exit()

		which = SYS_exit;
	}

	// invoke the appropriate syscall handler

	// should work in C99: _syscalls[which]( _current );
	(*_syscalls[which])( _current );

	// tell the PIC we're done

	__outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** Second-level syscall handlers
**
** All have this prototype:
**
**      static void _sys_NAME( pcb_t * );
**
** Those which return results to the calling user do so through the
** context save area pointed to by the supplied PCB.  This is also
** where incoming parameters to the syscall are found.
*/

/*
** _sys_exit - terminate the calling process
**
** implements:  void exit( void );
**
** does not return
*/

static void _sys_exit( pcb_t *pcb ) {
	
	// tear down the PCB structure

	_stack_dealloc( pcb->stack );
	_pcb_dealloc( pcb );

	// if this was the current process, we need a new one

	if( pcb == _current ) {
		_dispatch();
	}
}

/*
** _sys_spawnp - create a new process running the specified program
**
** implements:  int spawnp( void (*entry)(void), prio );
**
** returns:
**	pid of new process in original process, or -1 on error
*/

static void _sys_spawnp( pcb_t *pcb ) {
	pcb_t *new;

	// farm out all the work to this supporting routine

	new = _create_process( (uint32_t) ARG(1,pcb->context),
				(uint32_t) ARG(2,pcb->context) );

	if( new == NULL ) {

		// it failed - tell the parent
		RET(pcb->context) = -1;


	} else {

		// it succeeded - set the parent PID

		new->ppid = pcb->pid;

		// tell the parent
		RET(pcb->context) = new->pid;

		// schedule the child

		_schedule( new );
	}
}


/*
** _sys_sleep - put the current process to sleep for some length of time
**
** implements:	void sleep(uint32_t ms);
**
** If the sleep time (in milliseconds) is 0, just preempts the process;
** otherwise, puts it onto the sleep queue for the specified length of
** time.
*/

static void _sys_sleep( pcb_t *pcb ) {
	uint32_t sleeptime = (uint32_t) ARG(1,pcb->context);
	
	// if the desired sleep time is 0, treat this as a yield()

	if( sleeptime == 0 ) {

		// just preempt the process
		_schedule( pcb );

	} else {

		// calculate the wakeup time for the process
		pcb->wakeup = _system_time + MS_TO_TICKS(sleeptime);

		// mark it as sleeping
		pcb->state = STATE_SLEEPING;

		// add it to the sleep queue
		_queue_insert( _sleeping, (void *)pcb, (void *) pcb->wakeup );

	}
	
	// no current process - pick another one

	_dispatch();
}

/*
** _sys_get_process_info - retrieve information about a process
**
** implements:	int get_process_info( int code, int16_t pid );
**
** returns:
**	the desired information, or -1 if the PID couldn't be
**	located or the code wasn't recognized
*/

static void _sys_get_process_info( pcb_t *pcb ) {
	int code = (int) ARG(1,pcb->context);
	int pid = (int) ARG(2,pcb->context);

	pcb_t *target;

	// if PID is 0, use the current process;
	// else, find the desired one

	if( pid == 0 ) {

		target = _current;

	} else {

		target = _pcb_find( pid );

		// did we find it?
		if( target == NULL ) {
			RET(pcb->context) = -1;   // NO!
			return;
		}

	}

	// produce the desired information

	switch( code ) {

		case INFO_PID:
			RET(pcb->context) = target->pid;
			break;

		case INFO_PPID:
			RET(pcb->context) = target->ppid;
			break;

		case INFO_STATE:
			RET(pcb->context) = target->state;
			break;

		case INFO_WAKEUP:
			RET(pcb->context) = target->wakeup;
			break;

		case INFO_PRIO:
			RET(pcb->context) = target->prio;
			break;

		case INFO_QUANTUM:
			RET(pcb->context) = target->quantum;
			break;

		case INFO_DEF_QUANTUM:
			RET(pcb->context) = target->default_quantum;
			break;

		default:
			RET(pcb->context) = -1;
	}

}

/*
** _sys_get_system_info - retrieve information about the system
**
** implements:	int get_system_info( int code );
**
** returns:
**	the desired information, or -1 if the code wasn't recognized
*/

static void _sys_get_system_info( pcb_t *pcb ) {
	int code = (int) ARG(1,pcb->context);

	// produce the desired information

	switch( code ) {

		case SYSINFO_TIME:
			RET(pcb->context) = _system_time;
			break;

		case SYSINFO_NUM_PROCS:
			RET(pcb->context) = _system_active;
			break;

		case SYSINFO_MAX_PROCS:
			RET(pcb->context) = N_PROCS;
			break;

		default:
			RET(pcb->context) = -1;
	}

}

/*
** _sys_read - read bytes from the specified input channel
**
** implements:	int read( int fd, void *buf, int count );
**
** reads up to 'count' bytes or how many are available,
** whichever is smaller
**
** returns:
**	the count of characters read in
*/

static void _sys_read( pcb_t *pcb ) {
	int fd = (int) ARG(1,pcb->context);
	char *buf = (char *) ARG(2, pcb->context);
	int count = (int) ARG(3,pcb->context);
	int n;
	int (*qlength)(void);
	int (*getchar)(void);

	// set function pointers based on the stream

	if( fd == FD_CONSOLE ) {
		qlength = c_input_queue;
		getchar = c_getchar;
	} else if( fd == FD_SIO ) {
		qlength = _sio_input_queue;
		getchar = _sio_readc;
	} else {
		RET(pcb->context) = -1;
		return;
	}

	n = qlength();

	// if there are characters, read in the desired number of them

	if( n > 0 ) {

		// don't read more than the user wants, or more
		// characters than there are in the buffer

		if( n > count ) {
			n = count;
		} else {
			count = n;
		}

		while( count-- ) {
			*buf++ = getchar();
		}

	}

	RET(pcb->context) = n;

}

/*
** _sys_write - write bytes to the specified output channel
**
** implements:	int write( int fd, void *buf, int count );
**
** if 'count' is 0, write out a NUL-terminated buffer;
** otherwise, write 'count' bytes
**
** returns:
**	the count of characters written
*/

static void _sys_write( pcb_t *pcb ) {
	int fd = (int) ARG(1,pcb->context);
	char *buf = (char *) ARG(2,pcb->context);
	int count = (int) ARG(3,pcb->context);

	if( fd == FD_CONSOLE ) {

		if( count == 0 ) {
			RET(pcb->context) = c_puts( buf );
		} else {
			c_putbuf( buf, count );
			RET(pcb->context) = count;
		}

	} else if( fd == FD_SIO ) {

		if( count == 0 ) {
			RET(pcb->context) = _sio_puts( buf );
		} else {
			_sio_writes( buf, count );
			RET(pcb->context) = count;
		}
	
	} else {	// bad parameter!

		RET(pcb->context) = -1;

	}

}

/*
** PUBLIC FUNCTIONS
*/

/*
** _sys_modinit()
**
** initialize the syscall module
*/

void _sys_modinit( void ) {

	// allocate and initialize the sleep queue

	if( _queue_alloc(&_sleeping,1) != 1 ||
	    _sleeping == NULL ) {
		_kpanic( "_sys_init", "sleep queue alloc failed" );
	}

	_queue_init( _sleeping, _compare_time );

	/*
	** Set up the syscall jump table.  We do this here
	** to ensure that the association between syscall
	** code and function address is correct even if the
	** codes change.
	*/

	_syscalls[ SYS_exit ]             = _sys_exit;
	_syscalls[ SYS_spawnp ]           = _sys_spawnp;
	_syscalls[ SYS_sleep ]            = _sys_sleep;
	_syscalls[ SYS_read ]             = _sys_read;
	_syscalls[ SYS_write ]            = _sys_write;
	_syscalls[ SYS_get_process_info ] = _sys_get_process_info;
	_syscalls[ SYS_get_system_info ]  = _sys_get_system_info;

	// install our ISR

	__install_isr( INT_VEC_SYSCALL, _sys_isr );

	c_puts( " SYSCALL" );
}
