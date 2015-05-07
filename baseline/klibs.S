/*
** sccs id:	@(#)klibs.S	1.1	4/17/15
**
** File:	klibs.S
**
** Author:	CSCI-452 class of 20145
**
** Contributor:
**
** Description:	Kernel library assembly-language implementations
*/

#define	__SP_KERNEL__
#define	__SP_ASM__

/*
** _get_rbp - return current contents of RBP at the time of the call
**
** Could be used, e.g., by _kpanic to print a traceback
*/

	.globl	_get_rbp
_get_rbp:
	movq	%rbp, %rax
	ret
