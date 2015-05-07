/*
** SCCS ID:	@(#)stack.c	1.1	4/17/15
**
** File:	stack.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Stack-related implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include "stack.h"
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

static queue_t _free_stacks;		// queue of available stacks
static stack_t _stacks[ N_STACKS ];	// all the stacks in the system

/*
** PUBLIC GLOBAL VARIABLES
*/

stack_t _system_stack;		// separate stack for the OS itself
uint32_t *_system_esp;		// OS %ESP value

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _stack_modinit()
**
** initializes all stack-related data structures
*/

void _stack_modinit( void ) {
	
	// allocate and clear the free stack queue

	if( _queue_alloc(&_free_stacks,1) != 1 ||
	    _free_stacks == NULL ) {
		_kpanic( "_stack_modinit", "NULL free stack queue" );
	}

	_queue_init( _free_stacks, NULL );
	
	// "free" all the stacks

	for( int i = 0; i < N_STACKS; ++i ) {
		_stack_dealloc( &_stacks[i] );
	}
	
	// report that we have finished

	c_puts( " STACK" );
}

/*
** _stack_alloc()
**
** allocate a stack structure
**
** returns a pointer to the stack, or NULL on failure
*/

stack_t *_stack_alloc( void ) {
	stack_t *stack;

	// pull the first available stack off the free queue
	stack = (stack_t *) _queue_remove( _free_stacks );

	return( stack );
}

/*
** _stack_dealloc(stack)
**
** deallocate a stack, putting it into the set of available stacks
*/

void _stack_dealloc( stack_t *stack ) {
	
	// sanity check:  avoid deallocating a NULL pointer
	if( stack == NULL ) {
		// should this be an error?
		return;
	}

	// clear the stack

	_memset( (void *) stack, sizeof(stack_t), 0 );

	// return the stack to the free list

	_queue_insert( _free_stacks, (void *) stack, 0 );
}
