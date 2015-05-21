/*
** SCCS ID:	@(#)BuildImage.c	1.4	10/16/11
**
** File:	BuildImage.c
**
** Author:	K. Reek
**
** Contributor: Jon Coles, Warren R. Carithers, Garrett C. Smith
**
** Description:	Modify the bootstrap image to include the information
**		on the programs to be loaded, and produce the file
**		that contains the concatenation of these programs.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define	TRUE	1
#define	FALSE	0

char	*progname;		/* invocation name of this program */
char	*bootstrap_filename;	/* path of file holding bootstrap program */
char	*output_filename;	/* path of disk image file */
FILE	*out;			/* output stream for disk image file */

/*
** Array into which program information will be stored, starting at the
** end and moving back toward the front.  The array is sized at 512
** bytes, which is guaranteed to be larger than the maximum possible
** space available for this stuff in the bootstrap image.  Thus, the
** bootstrap image itself is the only limiting factor on how many program
** sections can be loaded.
*/
#define	N_INFO	( 512 / sizeof( short ) )
short	info[ N_INFO ];
int	n_info = N_INFO;

void quit( char *msg, int call_perror ) {
	if( msg != NULL ){
		fprintf( stderr, "%s: ", progname );
		if( call_perror ){
			perror( msg );
		}
		else {
			fprintf( stderr, "%s\n", msg );
		}
	}
	if( output_filename != NULL ){
		unlink( output_filename );
	}
	exit( EXIT_FAILURE );
}

char	usage_error_msg[] =
  "\nUsage: %s -b bootstrap_file -o output_file { prog_file load_address } ...\n\n"
  "\tThere must be at least one program file and load_address.\n\n"
  "\tLoad addresses may be specified either as 32-bit quantities in hex,\n"
  "\tdecimal or octal (e.g. 0x10c00, 68608, 0206000 are all equivalent).\n\n";

void usage_error( void ){
	fprintf( stderr, usage_error_msg, progname );
	quit( NULL, FALSE );
}

int copy_file( FILE *in ){
	char	buf[ 512 ];
	unsigned n_bytes;

	/*
	** Copy the file to the output, being careful that the
	** last block is padded with null bytes.
	*/
	unsigned n_sectors = 0;
	while( (n_bytes = fread( buf, 1, sizeof( buf ), in )) > 0 ){
		if( n_bytes < sizeof( buf ) ){
			for( unsigned i = n_bytes; i < sizeof( buf ); i += 1 ){
				buf[ i ] = '\0';
			}
		}
		if( fwrite( buf, 1, sizeof( buf ), out ) != sizeof( buf ) ){
			quit( "Write failed or was wrong size", FALSE );
		}
		n_sectors += 1;
	}
	return n_sectors;
}

void process_file( char *name, char *addr ){
	FILE	*in;
	int	n_sectors;
	long	address, address_end;
	short	segment, offset;
	int	valid_address;

	/*
	** Open the input file.
	*/
	in = fopen( name, "rb" );
	if( in == NULL ){
		quit( name, TRUE );
	}

	/*
	** Copy the file to the output, being careful that the
	** last block is padded with null bytes.
	*/
	n_sectors = copy_file( in );
	fclose( in );

	/*
	** Decode the address they gave us.
	*/
	valid_address = FALSE;
	char	*unused;

	address = strtol( addr, &unused, 0 );
	segment = (short)( address >> 4 );
	offset = (short)( address & 0xf );
	valid_address = *unused == '\0' && (
	    ( address <= 0x0009ffff ) ||
	    ( address >= 0x00100000 && address <= 0x00dfffff ) );
	if( !valid_address ){
		fprintf( stderr, "%s: Invalid address: %s\n", progname, addr );
		quit( NULL, FALSE );
	}

	/*
	** Make sure the program will fit!
	*/
	address_end = address + n_sectors * 512;
	if(
	    ( address_end > 0x0009ffff && address_end < 0x00100000 ) ||
	    ( address_end > 0x00dfffff )
	) {
		fprintf( stderr, "Program %s too large to start at 0x%08x\n",
		    name, (unsigned int) address );
		quit( NULL, FALSE );
	}
	if( n_info < 3 ){
		quit( "Too many programs!", FALSE );
	}


	/*
	** Looks good: report and store the information.
	*/
	fprintf( stderr, "%s: %d sectors, loaded at 0x%x\n",
	    name, n_sectors, (unsigned int) address );

	info[ --n_info ] = n_sectors;
	info[ --n_info ] = segment;
	info[ --n_info ] = offset;
}

/*
** Global variables set by getopt()
*/

extern int optind, optopt;
extern char *optarg;

void process_args( int ac, char **av ) {
	int c;

	while( (c=getopt(ac,av,":d:o:b:")) != EOF ) {

		switch( c ) {

			case ':':	/* missing arg value */
				fprintf( stderr, "missing operand after -%c\n", optopt );
				/* FALL THROUGH */

			case '?':	/* error */
				usage_error();

			case 'b':	/* -b bootstrap_file */
				bootstrap_filename = optarg;
				break;
			case 'o':	/* -o output_file */
				output_filename = optarg;
				break;

			default:
				usage_error();

		}

	}

	if( !bootstrap_filename ) {
		fprintf( stderr, "%s: no bootstrap file specified\n", progname );
		exit( 2 );
	}

	if( !output_filename ) {
		fprintf( stderr, "%s: no disk image file specified\n", progname );
		exit( 2 );
	}

	if( optind > (ac - 2 ) ) {
		usage_error();
	}

}

int main( int ac, char **av ) {
	FILE	*bootimage;
	int	bootimage_size;
	unsigned n_bytes, n_words;
	short	existing_data[ N_INFO ];

	/*
	** Save the program name for error messages
	*/
	progname = strrchr( av[ 0 ], '/' );
	if( progname != NULL ){
		progname++;
	}
	else {
		progname = av[ 0 ];
	}

	/*
	** Process arguments
	*/
	process_args( ac, av );

	/*
	** Open the output file
	*/

	out = fopen( output_filename, "wb+" );
	if( out == NULL ){
		quit( output_filename, TRUE );
	}

	/*
	** Open the bootstrap file and copy it to the output image.
	*/
	bootimage = fopen( bootstrap_filename, "rb" );
	if( bootimage == NULL ){
		quit( bootstrap_filename, TRUE );
	}
	bootimage_size = copy_file( bootimage ) * 512;
	fclose( bootimage );

	/*
	** Process the programs one by one
	*/
	ac -= optind;
	av += optind;
	while( ac >= 2 ){
		process_file( av[ 0 ], av[ 1 ] );
		ac -= 2; av += 2;
	}

	/*
	** Check for oddball leftover argument
	*/
	if( ac > 0 ){
		usage_error();
	}

	/*
	** Seek to where this array must begin and read what's already there.
	*/
	n_words = ( N_INFO - n_info );
	n_bytes = n_words * sizeof( info[ 0 ] );
	fseek( out, bootimage_size - n_bytes, SEEK_SET );
	if( fread( existing_data, sizeof( info[ 0 ] ), n_words, out ) != n_words ){
		quit( "Read from boot image failed or was too short", FALSE );
	}

	for( unsigned i = 0; i < n_words; i += 1 ){
		if( existing_data[ i ] != 0 ){
			quit( "Too many programs to load!", FALSE );
		}
	}

	/*
	** We know that we're only overwriting zeros at the end of
	** the bootstrap image, so it is ok to go ahead and do it.
	*/
	fseek( out, bootimage_size - n_bytes, SEEK_SET );
	if( fwrite( info + n_info, sizeof( info[ 0 ] ), n_words, out ) != n_words ){
		quit( "Write to boot image failed or was too short", FALSE );
	}

	fclose( out );

	return EXIT_SUCCESS;

}
