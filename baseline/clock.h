/*
** SCCS ID:	@(#)clock.h	1.1	4/17/15
**
** File:	clock.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Clock module
*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

// clock interrupts per second

#define	CLOCK_FREQUENCY		1000

// standard process quantum (in ticks)

#define	QUANTUM_DEFAULT		10

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// pseudo function to convert seconds to milliseconds

#define	SECONDS_TO_MS(n)	((n) * 1000)

// pseudo function to convert milliseconds to clock ticks
// (currently, a no-op, as the base clock rate is 1000 ticks/sec)

#define	MS_TO_TICKS(n)		((n))

// pseudo function to convert seconds to ticks

#define	SECONDS_TO_TICKS(n)	(MS_TO_TICKS(SECONDS_TO_MS((n))))

// pseudo function to convert ticks to (truncated) seconds

#define	TICKS_TO_SECONDS(n)	((n) / CLOCK_FREQUENCY)

// pseudo function to convert ticks to (rounded up) seconds

#define	TICKS_TO_ROUNDED_SECONDS(n)	(((n)+(CLOCK_FREQUENCY-1)) / CLOCK_FREQUENCY)

/*
** Types
*/

/*
** Globals
*/

extern uint32_t	_system_time;	// the current system time

/*
** Prototypes
*/

/*
** _clock_modinit()
**
** initialize the clock module
*/

void _clock_modinit( void );

#endif

#endif
