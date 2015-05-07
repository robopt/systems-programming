/*
** SCCS ID:	@(#)common.h	1.1	4/9/14
**
** File:	common.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Standard includes needed in all C source files
*/

#ifndef _COMMON_H_
#define _COMMON_H_

// correct way to define NULL

#define NULL	0

// maximum number of processes the system will support

#define	N_PROCS	25

// maximum number of queues the system will support

#define	N_QUEUES 10

// file descriptors for built-in devices

#define	FD_CONSOLE	0
#define	FD_SIO		1

// information specifiers for get_process_info()

#define	INFO_PID		0
#define	INFO_PPID		1
#define	INFO_STATE		2
#define	INFO_WAKEUP		3
#define	INFO_PRIO		4
#define	INFO_QUANTUM		5
#define	INFO_DEF_QUANTUM	6

// information specifiers for get_system_info()

#define	SYSINFO_TIME		0
#define	SYSINFO_NUM_PROCS	1
#define	SYSINFO_MAX_PROCS	2

#ifndef __SP_ASM__

// only pull these in if we're not in assembly language

#include "types.h"

#ifdef __SP_KERNEL__

// OS needs the kernel library headers and the system headers

#include "c_io.h"
#include "support.h"
#include "system.h"
#include "klib.h"

#else

// User code needs only the user library headers

#include "ulib.h"

#endif

#endif

#endif
