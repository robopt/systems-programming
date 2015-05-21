/*
** SCCS ID:	@(#)klib.h	1.1	4/17/15
**
** File:	klib.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Kernel support library declarations
*/

#ifndef _KLIB_H_
#define _KLIB_H_

#include "types.h"

/*
** General (C and/or assembly) definitions
*/

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

/*
** Prototypes
*/

/*
** _get_ebp - return current contents of EBP at the time of the call
**
** Could be used, e.g., by _kpanic to print a traceback
*/

uint32_t _get_ebp( void );

/*
** _put_char_or_code( ch )
**
** prints the character on the console, unless it is a non-printing
** character, in which case its hex code is printed
*/

void _put_char_or_code( int ch );

/*
** _memset - initialize all bytes of a block of memory to a value
**
** usage:  _memset( buffer, length, value )
*/

void _memset( register uint8_t *buf, register uint32_t len, register uint8_t value );

/*
** _kpanic - kernel-level panic routine
**
** usage:  _kpanic( module, msg )
**
** Prefix routine for __panic() - can be expanded to do other things
** (e.g., printing a stack traceback)
*/

void _kpanic( char *mod, char *msg );

#endif

#endif
