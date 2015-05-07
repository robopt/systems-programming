#
# SCCS ID: %W%	%G%
#
# Makefile to control the compiling, assembling and linking of
# standalone programs in the DSL.  Used for both individual
# interrupt handling assignments and the SP baseline OS (with
# appropriate tweaking).
#

#
# User supplied files
#
U_C_SRC = clock.c klibc.c process.c queue.c scheduler.c sio.c \
	stack.c syscall.c system.c ulibc.c user.c

U_C_OBJ = clock.o klibc.o process.o queue.o scheduler.o sio.o \
	stack.o syscall.o system.o ulibc.o user.o

U_S_SRC = klibs.S ulibs.S

U_S_OBJ = klibs.o ulibs.o

U_H_SRC = clock.h klib.h process.h queue.h scheduler.h sio.h \
	stack.h syscall.h system.h types.h ulib.h user.h

U_LIBS	=

#
# User compilation/assembly definable options
#
#	CLEAR_BSS_SEGMENT	include code to clear all BSS space
#	ISR_DEBUGGING_CODE	include context restore debugging code
#	REPORT_MYSTERY_INTS	print a message on interrupt 0x27
#	SP_OS_CONFIG		enable SP OS-specific startup variations
#
USER_OPTIONS = -DDEBUG -DDUMP_QUEUES -DCLEAR_BSS_SEGMENT -DISR_DEBUGGING_CODE -DSP_OS_CONFIG

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory
#
INCLUDES = -I.

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
# CPPFLAGS = $(USER_OPTIONS) -nostdinc -I- $(INCLUDES)
CPPFLAGS = $(USER_OPTIONS) -nostdinc $(INCLUDES)

CC = gcc
CFLAGS = -m32 -std=c99 -fno-stack-protector -fno-builtin -Wall -Wextra -pedantic -Wstrict-prototypes $(CPPFLAGS)

AS = as -Wall
ASFLAGS = --32

LD = ld
LDFLAGS = -melf_i386

#
# Transformation rules - these ensure that all compilation
# flags that are necessary are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file.
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#

.SUFFIXES:	.S .b

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.S.s:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

# Binary/source file for system bootstrap code

BOOT_OBJ = bootstrap.b
BOOT_SRC = bootstrap.S

# Assembly language object/source files

S_OBJ = startup.o isr_stubs.o $(U_S_OBJ)
S_SRC =	startup.S isr_stubs.S $(U_S_SRC)

# C object/source files

C_OBJ =	c_io.o support.o $(U_C_OBJ)
C_SRC =	c_io.c support.c $(U_C_SRC)

# Collections of files

OBJECTS = $(S_OBJ) $(C_OBJ)

SOURCES = $(BOOT_SRC) $(S_SRC) $(C_SRC)

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

usb.image: bootstrap.b prog.b prog.nl prog.nlf BuildImage
	./BuildImage -d usb -o usb.image -b bootstrap.b prog.b 0x10000

floppy.image: bootstrap.b prog.b prog.nl BuildImage
	./BuildImage -d floppy -o floppy.image -b bootstrap.b prog.b 0x10000

prog.out: $(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.out $(OBJECTS)

prog.o:	$(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.o -Ttext 0x10000 $(OBJECTS) $(U_LIBS)

prog.b:	prog.o
	$(LD) $(LDFLAGS) -o prog.b -s --oformat binary -Ttext 0x10000 prog.o

#
# Targets for copying bootable image onto boot devices
#

floppy:	floppy.image
	/usr/local/dcs/bin/fcopy floppy.image
#	old:  dd if=floppy.image of=/dev/fd0

usb:	usb.image
	/usr/local/dcs/bin/dcopy usb.image
#	old:  dd if=usb.image of=/local/devices/disk

#
# Special rule for creating the modification and offset programs
#
# These are required because we don't want to use the same options
# as for the standalone binaries.
#

BuildImage:	BuildImage.c
	$(CC) -o BuildImage BuildImage.c

#
# won't compile on 14.04 x86_64 due to missing sys/cdefs.h file
# (probably in some package somewhere that hasn't been installed)
#
# Offsets:	Offsets.c
#	$(CC) -m32 $(INCLUDES) -o Offsets Offsets.c

#
# Clean out this directory
#

clean:
	rm -f *.nl *.nlf *.lst *.b *.o *.image *.dis BuildImage Offsets

#
# Create a printable namelist from the prog.o file
#

prog.nl: prog.o
	nm -Bng prog.o | pr -w80 -3 > prog.nl

prog.nlf: prog.o
	nm -Bn prog.o | pr -w80 -3 > prog.nlf

#
#       makedepend is a program which creates dependency lists by
#       looking at the #include lines in the source files
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bootstrap.o: bootstrap.h
startup.o: bootstrap.h
isr_stubs.o: bootstrap.h
ulibs.o: syscall.h common.h
c_io.o: c_io.h startup.h support.h x86arch.h
support.o: startup.h support.h c_io.h x86arch.h bootstrap.h
clock.o: x86arch.h startup.h clock.h types.h process.h stack.h queue.h
clock.o: scheduler.h sio.h syscall.h common.h
klibc.o: common.h
process.o: common.h process.h types.h clock.h stack.h queue.h
queue.o: common.h types.h stack.h process.h clock.h scheduler.h queue.h
scheduler.o: common.h scheduler.h types.h process.h clock.h stack.h queue.h
sio.o: common.h sio.h queue.h types.h process.h clock.h stack.h scheduler.h
sio.o: system.h startup.h ./uart.h x86arch.h
stack.o: common.h stack.h types.h queue.h
syscall.o: common.h syscall.h process.h types.h clock.h stack.h queue.h
syscall.o: scheduler.h sio.h support.h startup.h x86arch.h
system.o: common.h system.h types.h process.h clock.h stack.h bootstrap.h
system.o: syscall.h sio.h queue.h scheduler.h user.h ulib.h
ulibc.o: common.h ulib.h types.h process.h clock.h stack.h
user.o: common.h ulib.h types.h process.h clock.h stack.h user.h c_io.h
