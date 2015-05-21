/*
** SCCS ID:	@(#)klibc.c	1.1	4/17/15
**
** File:	klibc.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	C implementations of kernel library routines
*/

#define	__SP_KERNEL__

#include "common.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _put_char_or_code( ch )
**
** prints the character on the console, unless it is a non-printing
** character, in which case its hex code is printed
*/

void _put_char_or_code( int ch ) {

	if( ch >= ' ' && ch < 0x7f ) {
		c_putchar( ch );
	} else {
		c_printf( "\\x%02x", ch );
	}
}

/*
** _memset - initialize all bytes of a block of memory to a value
**
** usage:  _memset( buffer, length, value )
*/

void _memset( register uint8_t *buf, register uint32_t len, uint8_t value ) {

	while( len-- ) {
		*buf++ = value;
	}

}

/*
** _kpanic - kernel-level panic routine
**
** usage:  _kpanic( module, msg );
**
** Prefix routine for __panic() - can be expanded to do other things
** (e.g., printing a stack traceback)
**
** 'module' argument is always printed; 'msg' argument is printed
** if it isn't NULL, followed by a newline
*/

void _kpanic( char *module, char *msg ) {

	c_puts( "\n\n***** KERNEL PANIC *****\n\n" );
	c_printf( "Module: %s\n", module );
	if( msg != NULL ) {
		c_puts( msg );
		c_putchar( '\n' );
	}
	//
	// This might be a good place to do a stack frame
	// traceback
	//

	__panic( "KERNEL PANIC" );

}
