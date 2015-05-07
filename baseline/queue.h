/*
** SCCS ID:	@(#)queue.h	1.1	4/17/15
**
** File:	queue.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Queue module declarations
*/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "types.h"
#include "queue.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
**
** Our queues are generic, self-ordering queues.  Each queue has an
** associated ordering routine which is used by the insertion 
** routine to order the queue elements.  This allows us to have
** different sorting criteria for different queues, but manage them
** with one set of functions.
*/

// prototype for the ordering function

typedef int (*comparef_t)( void *, void * );

/*
** The queue itself is opaque outside of this module, so we will
** use an anonymous struct declaration
*/

struct queue;
typedef struct queue *queue_t;

/*
** Globals
*/

/*
** Prototypes
*/

/*
** _compare_time()
**
** compare two qnode keys as "time" values
*/

int _compare_time( void *qn1, void *qn2 );

/*
** _queue_modinit()
**
** initialize the queue module
*/

void _queue_modinit( void );

/*
** _queue_alloc()
**
** allocate some number of queues
**
** returns the count of allocated queues
*/

int _queue_alloc( queue_t locs[], int num );

/*
** _queue_dealloc()
**
** deallocate a queue
*/

void _queue_dealloc( queue_t queue );

/*
** _queue_init(que,fcn)
**
** initialize the specified queue 
*/

void _queue_init( queue_t que, comparef_t fcn );

/*
** _queue_insert(que,data)
**
** insert the supplied data value into the queue, ordering the
** queue using its built-in comparison function
*/

void _queue_insert( queue_t queue, void *data, void *key );

/*
** _queue_remove(que)
**
** remove the first element from the queue
**
** returns the thing that was removed, or NULL
*/

void *_queue_remove( queue_t queue );

/*
** _queue_kpeek(que)
**
** peek at the key of the first element in the queue
**
** returns the key, or NULL
*/

void *_queue_kpeek( queue_t queue );

/*
** _queue_dpeek(que)
**
** peek at the data item of the first element in the queue
**
** returns a pointer to the data item, or NULL
*/

void *_queue_dpeek( queue_t queue );

/*
** _queue_empty(que)
**
** indicate whether or not the specified queue is empty
*/

bool_t _queue_empty( queue_t queue );

/*
** _queue_size(que)
**
** return the number of elements in the specified queue
*/

uint32_t _queue_size( queue_t queue );

/*
** _queue_dump(which,queue)
**
** dump the contents of the specified queue to the console
*/

void _queue_dump( char *which, queue_t queue );

#endif

#endif
