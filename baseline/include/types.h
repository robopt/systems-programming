/*
** SCCS ID:	@(#)types.h	1.1	4/17/15
**
** File:	types.h
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	General data type declarations
*/

#ifndef _TYPES_H_
#define _TYPES_H_

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

// size-specific integer types

typedef char		int8_t;
typedef unsigned char	uint8_t;
typedef int8_t		byte_t;
typedef uint8_t		ubyte_t;

typedef short		int16_t;
typedef unsigned short	uint16_t;
typedef int16_t		short_t;
typedef uint16_t	ushort_t;

typedef long		int32_t;
typedef unsigned long	uint32_t;

typedef _Bool		bool_t;

#ifdef __SP_KERNEL__

/*
** OS-only type declarations
*/

#endif

/*
** Globals
*/

/*
** Prototypes
*/

#endif

#endif
