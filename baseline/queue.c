/*
** SCCS ID:	@(#)queue.c	1.1	4/17/15
**
** File:	queue.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Queue-related implementations
*/

#define	__SP_KERNEL__

#include "common.h"
#include "types.h"
#include "stack.h"
#include "process.h"
#include "scheduler.h"

/*
** PRIVATE DEFINITIONS
*/

// number of qnodes to create
// need one per:  PCB, stack, queue
// add a fudge factor

#define N_QNODES	(N_PROCS + N_STACKS + N_QUEUES + 3)

/*
** PRIVATE DATA TYPES
*/

// structure of a qnode

typedef struct qnode {
	void *key;		// sorting key
	void *data;		// data queued
	struct qnode *next;	// link to successor
} qnode_t;

// now we can include queue.h to get the queue_t type

#include "queue.h"

// the actual queue definition

struct queue {
	qnode_t *first;		// head pointer
	qnode_t *last;		// tail pointer
	comparef_t compare;	// comparison function
	uint32_t size;		// queue length
};

/*
** PRIVATE GLOBAL VARIABLES
*/

static qnode_t *_free_qnodes;		// list of available qnodes
static qnode_t _qnodes[N_QNODES];	// all qnodes in the system

static queue_t _free_queues;		// queue of available queues
static struct queue _queues[N_QUEUES];	// all available queues

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** _compare_time()
**
** compare two keys as "time" values
*/

int _compare_time( void *qn1, void *qn2 ) {
	uint32_t k1 = (uint32_t) qn1;
	uint32_t k2 = (uint32_t) qn2;

	return( k1 - k2 );
}

/*
** _qnode_dealloc()
**
** return a qnode to the free pool
*/

static void _qnode_dealloc( qnode_t *node ) {

#ifdef DEBUG
	if( node == NULL ) {
		_kpanic( "_qnode_dealloc", "NULL node" );
	}
#endif
	node->next = _free_qnodes;
	_free_qnodes = node;
}

/*
** _qnode_alloc()
**
** obtain a qnode from the free pool
*/

static qnode_t *_qnode_alloc( void ) {
	qnode_t *node;
	
	node = _free_qnodes;
	if( node != NULL ) {
		_free_qnodes = node->next;
		node->next = NULL;
	}
	
	return( node );
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _queue_modinit()
**
** initialize the queue module
*/

void _queue_modinit( void ) {

	_free_qnodes = NULL;
	for( int i = 0; i < N_QNODES; ++i ) {
		_qnode_dealloc( &_qnodes[i] );
	}
	
	// steal the first queue for _free_queues

	_free_queues = &_queues[0];
	_queue_init( _free_queues, 0 );

	// add the rest to the free queue

	for( int i = 1; i < N_QUEUES; ++i ) {
		_queue_dealloc( &_queues[i] );
	}
	
	c_puts( " QUEUE" );
}

/*
** _queue_alloc()
**
** allocate a queue
**
** returns a pointer to the allocated queue, or NULL
*/

int _queue_alloc( queue_t locs[], int num ) {
	int i;

	for( i = 0; i < num; ++i ) {

		// if none available, we're done

		if( _queue_empty(_free_queues) ) {
			break;
		}

		// allocate one
		locs[i] = _queue_remove( _free_queues );

		// if it didn't work, we're done
		if( locs[i] == NULL ) {
			break;
		}
	}

	return( i );
}

/*
** _queue_dealloc()
**
** return a queue to the free list
*/

void _queue_dealloc( queue_t queue ) {
	
	// sanity check:  avoid deallocating a NULL pointer
	if( queue == NULL ) {
		// should this be an error?
		return;
	}

	// clear the queue

	_memset( (void *) queue, sizeof(struct queue), 0 );

	// put it back on the free list

	_queue_insert( _free_queues, (void *) queue, 0 );
}

/*
** _queue_init()
**
** initialize the supplied queue
*/

void _queue_init( queue_t queue, comparef_t compare ) {

#ifdef DEBUG
	if( queue == NULL ) {
		_kpanic( "_queue_init", "NULL queue" );
	}
#endif

	queue->first = NULL;
	queue->last = NULL;
	queue->size = 0;
	queue->compare = compare;
}

/*
** _queue_remove()
**
** remove the first element from the queue
**
** returns the thing that was removed, or NULL
*/

void *_queue_remove( queue_t queue ) {
	qnode_t *node;
	void *data;
	
#ifdef DEBUG
	if( queue == NULL ) {
		_kpanic( "_queue_remove", "NULL queue" );
	}
#endif

	if( queue->first == NULL ) {
		return( NULL );
	}
	
	node = queue->first;
	queue->first = node->next;
	if( queue->first == NULL ) {
		queue->last = NULL;
	}
	queue->size -= 1;
	
	data = node->data;
	
	_qnode_dealloc( node );
	
	return( data );
}

/*
** _queue_insert()
**
** insert the supplied data value into the queue, ordering the
** queue using its built-in comparison function
*/

void _queue_insert( queue_t queue, void *data, void *key ) {
	qnode_t *node;
	
#ifdef DEBUG
	if( queue == NULL ) {
		_kpanic( "_queue_insert", "NULL queue" );
	}
#endif
	
	node = _qnode_alloc();
	if( node == NULL ) {
		_kpanic( "_queue_insert", "NULL qnode" );
	}
	
	node->data = data;
	node->key = key;

	if( _queue_empty(queue) ) {
		queue->first = node;
		queue->last = node;
		queue->size = 1;
		return;
	}
	
	if( queue->compare == NULL ) {
		queue->last->next = node;
		queue->last = node;
		queue->size += 1;
		return;
	}
	
	qnode_t *prev, *curr;
	
	prev = NULL;
	curr = queue->first;
	
	while( curr != NULL &&
		queue->compare(node->key,curr->key) >= 0 ) {
		
		prev = curr;
		curr = curr->next;
	}
	
	/*
	**  prev  curr  meaning
	**  ==============================================
	**  NULL  !NULL add at front:  new -> curr, first -> new
	**  NULL  NULL  error???  cannot happen???
	**  !NULL !NULL add in middle: new -> curr, prev -> new
	**  !NULL NULL  add at end:    new -> NULL, prev -> new, last -> new
	*/
	
	node->next = curr;
	if( prev == NULL ) {
		queue->first = node;
	} else {
		prev->next = node;
	}
	if( curr == NULL ) {
		queue->last = node;
	}
	
	queue->size += 1;
}

/*
** _queue_empty()
**
** determine whether or not the supplied queue is empty
*/

bool_t _queue_empty( queue_t queue ) {
	return( queue->first == NULL );
}

/*
** _queue_size()
**
** return the number of elements in the supplied queue
*/

uint32_t _queue_size( queue_t queue ) {
	return( queue->size );
}

/*
** _queue_kpeek()
**
** peek at the first element in the queue
**
** returns the key associated with that element
*/

void *_queue_kpeek( queue_t queue ) {

#ifdef DEBUG
	if( queue == NULL ) {
		_kpanic( "_queue_kpeek", "NULL queue" );
	}
#endif

	if( _queue_empty(queue) ) {
		return( NULL );
	}
	
	return( queue->first->key );
}

/*
** _queue_dpeek()
**
** peek at the first element in the queue
**
** returns the data associated with that element
*/

void *_queue_dpeek( queue_t queue ) {

#ifdef DEBUG
	if( queue == NULL ) {
		_kpanic( "_queue_dpeek", "NULL queue" );
	}
#endif

	if( _queue_empty(queue) ) {
		return( NULL );
	}
	
	return( queue->first->data );
}

/*
** _queue_dump(which,queue)
**
** dump the contents of the specified queue to the console
*/

void _queue_dump( char *which, queue_t queue ) {
	qnode_t *tmp;
	int i;

	c_printf( "%s: ", which );
	if( queue == NULL ) {
		c_puts( "NULL???" );
		return;
	}

	c_printf( "first %08x last %08x comp %08x (%d items)\n",
		  (uint32_t) queue->first, (uint32_t) queue->last,
		  (uint32_t) queue->compare, queue->size );
	
	if( _queue_size(queue) > 0 ) {
		c_puts( " data: " );
		i = 0;
		for( tmp = queue->first; tmp != NULL; tmp = tmp->next ) {
			c_printf( " [%x,%x]", (uint32_t) tmp->key,
				(uint32_t) tmp->data );
			if( ++i > 10 ) break;
		}
		if( tmp != NULL ) {
			c_puts( " ..." );
		}
		c_puts( "\n" );
	}

}
