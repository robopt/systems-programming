/*
** SCCS ID:	@(#)ulibc.c	1.1	4/17/15
**
** File:	ulibc.c
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	C implementations of user-level library functions
*/

#define	__SP_KERNEL__

#include "common.h"
#include "ulib.h"

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
** __default_exit__()
**
** default "return to" code for all user processes' main routines
*/

void __default_exit__( void ) {
	exit();
}

/*
** spawn()
**
** invoke spawnp() with the priority of the calling process
*/

int32_t spawn( void (*entry)(void) ) {
	return( spawnp(entry,get_process_info(INFO_PRIO,0)) );
}

/*
** Integer to string conversion routines
**
** Decimal conversion
**
** itos10() - convert an int into a decimal number
*/

static char *itos10x( char *buf, int value ) {
	int quotient;
	
	quotient = value / 10;
	if( quotient < 0 ) {
		quotient = 214748364;
		value = 8;
	}

	if( quotient != 0 ) {
		buf = itos10x( buf, quotient );
	}

	*buf++ = value % 10 + '0';
	return( buf );
}
		

int itos10( char *buf, int value ) {
	char *bp = buf;

	if( value < 0 ) {
		*bp++ = '-';
		value = -value;
	}

	bp = itos10x( bp, value );
	*bp = '\0';

	return( bp - buf );
}

static char mydigits[] = "0123456789ABCDEF";

/*
** Hex and octal conversion
**
** itos8() - convert an int into an octal number (1 to 11 digits)
*/

int itos8( char *buf, int value, int store_all ) {
	char *bp = buf;
	int val;

	// must deal with upper two bits separately
	val = (value & 0xc0000000) >> 30;
	value <<= 2;

	for( int i = 0; i < 11; ++i ) {
		if( i == 10 || val != 0 || store_all ) {
			store_all = 1;
			*bp++ = mydigits[ val & 0x7 ];
		}
		val = (value & 0xe0000000) << 29;
		value <<= 3;
	}

	*bp = '\0';

	return( bp - buf );
}

/*
** itos16() - convert and into into a hex number (1 to 8 digits)
*/

int itos16( char *buf, int value, int store_all ) {
	char *bp = buf;
	int val;

	for( int i = 0; i < 8; ++i ) {
		val = (value & 0xf0000000) >> 28;
		if( i == 7 || val != 0 || store_all ) {
			store_all = 1;
			*bp++ = mydigits[ val & 0xf ];
		}
		value <<= 4;
	}

	*bp = '\0';

	return( bp - buf );
}
