/*
** SCCS ID:	@(#)syscall.h	1.1	4/17/15
**
** File:	syscall.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	System call module definitions
*/

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// system call codes

#define	SYS_exit		0
#define	SYS_spawnp		1
#define	SYS_sleep		2
#define	SYS_read		3
#define	SYS_write		4
#define	SYS_get_process_info	5
#define	SYS_get_system_info	6

// number of "real" system calls

#define	N_SYSCALLS	7

// dummy system call code to test the syscall ISR

#define	SYS_bogus	(N_SYSCALLS+50)

// system call interrupt vector number

#define	INT_VEC_SYSCALL	0x80

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

#ifdef __SP_KERNEL__

#include "queue.h"

#include <x86arch.h>

/*
** OS only definitions
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

extern queue_t _sleeping;      // sleeping processes

/*
** Prototypes
*/

/*
** _sys_modinit()
**
** initializes all syscall-related data structures
*/

void _sys_modinit( void );

#endif

#endif

#endif
