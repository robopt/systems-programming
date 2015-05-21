/*
** SCCS ID:	@(#)user.h	1.1	4/17/15
**
** File:	user.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	User routines
*/

#ifndef _USER_H_
#define _USER_H_

/*
** General (C and/or assembly) definitions
*/

// delay loop counts

#define	DELAY_LONG	100000000
#define	DELAY_STD	  2500000
#define	DELAY_ALT	  4500000

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** User process controls.
**
** The comment field of these definitions contains a list of the
** system calls this user process uses.
**
** To spawn a specific user process from the initial process,
** uncomment its entry in this list.
**
** None of these user processes use spawnp() - init() does
** that, but these processes always use spawn().
*/

//	user	          baseline system calls in use
		// exit spwn slep read writ gpin gsin
#define	SPAWN_A	//  X    .    .    .    X    .    .
#define	SPAWN_B	//  X    .    .    .    X    .    .
#define	SPAWN_C	//  X    .    .    .    X    .    .
//#define	SPAWN_D	//  X    X    .    .    X    .    .
//#define	SPAWN_E	//  X    .    X    .    X    X    .
//#define	SPAWN_F	//  X    .    X    .    X    X    .
//#define	SPAWN_G	//  X    .    X    .    X    X    .
//#define	SPAWN_H	//  .    .    .    .    X    .    .
//// skipping user I
//#define	SPAWN_J	//  X    X    .    .    X    .    .
//#define	SPAWN_K	//  X    X    X    .    X    .    .
//#define	SPAWN_L	//  X    X    X    .    X    .    X
//#define	SPAWN_M	//  X    .    .    .    X    X    .
//#define	SPAWN_N	//  X    .    .    .    X    .    .
//// skipping user O
//#define	SPAWN_P	//  X    .    X    .    X    .    .
//#define	SPAWN_Q	//  X    .    .    .    X    .    .
//#define	SPAWN_R	//  X    .    X    X    X    .    .
// no user S
// no user T
// no user U
// no user V
#define SPAWN_NET

/*
** Users W-Z are spawned from other processes; they
** should never be spawned directly.
*/

// no user W
// user X:	//  X    .    .    .    X    X    .
// user Y:	//  X    .    X    .    X    .    .
// user Z:	//  X    .    .    .    X    X    .


/*
** Prototypes for externally-visible routines
*/

/*
** init - initial user process
*/

void init( void );

/*
** idle - what to run when there's nothing else to run
*/

void idle( void );

#endif

#endif
