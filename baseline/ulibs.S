/*
** sccs id:	@(#)ulibs.S	1.1	4/17/15
**
** File:	ulibs.S
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Assembly implementations of user-level library functions
*/

#define	__SP_ASM__

#include "syscall.h"

/*
** System call stubs
**
** All have the same structure:
**
**      move a code into EAX
**      generate the interrupt
**      return to the caller
**
** As these are simple "leaf" routines, we don't use
** the standard enter/leave method to set up a stack
** frame - that takes time, and we don't really need it.
*/

#define SYSCALL(name) \
	.globl	name              ; \
name:                             ; \
	movl	$SYS_##name, %eax ; \
	int	$INT_VEC_SYSCALL  ; \
	ret

SYSCALL(exit)
SYSCALL(spawnp)
SYSCALL(sleep)
SYSCALL(read)
SYSCALL(write)
SYSCALL(get_process_info)
SYSCALL(get_system_info)

// This is a bogus system call; it's here so that we can test
// our handling of out-of-range syscall codes in the syscall ISR.

SYSCALL(bogus)
