#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef BYTE
#define BYTE char
#endif
#ifndef BOOL
#define BOOL bool
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif
#ifndef nullptr
#define nullptr 0x0
#endif
#ifndef NULL
#define NULL 0
#endif

BOOL read_bytes( FILE* handle, size_t len, BYTE* buffer ) {
	if ( buffer == nullptr || len == 0 ) {
		return FALSE;
	}
	for ( size_t i = 0; i < len; i++ ) {
		BYTE iterative_byte = fgetc( handle );
		if ( feof( handle ) ) {
			return FALSE;
		}
		buffer[ i ] = iterative_byte;
	}
	return TRUE;
}

typedef struct png_chunk {
	fpos_t location; // Offset in file.
	int size; // Not including name or checksum.
	unsigned char* name;//[4];
	BYTE* data; // Allocated using calloc so free this when you're finished.
	unsigned int checksum; // CRC32.
} chunk, *pchunk;
typedef struct ihdr_data {
// | <--4--> | <--4--> | <---1---> | <----1----> | <----1----> | <---1---> | <---1---> |
// | width   | height  | bit-depth | colour-type | compression |   filter  | interlace |
//      4    +    4    +     1     +      1      +      1      +     1     +     1     = 13 bytes.
	int width;
	int height;
	BYTE bit_depth;
	BYTE colour_type;
	BYTE compression_type;
	BYTE filter_type;
	BYTE interlace_type;
} ihdr_data, *pihdr_data;

// If max_length == 0 we assume the developer intended to specify
// 'unlimited' and disregard the buffer's size.
BOOL read_chunk( FILE* handle, size_t max_length, chunk* buffer ) {
	// Assuming that the handle's
	// read position is currently
	// positioned at the start of
	// the chunk.
	chunk current_chunk = { .size = 0, .data = nullptr, .checksum = 0x0,
		.location = 0 };

	if ( fgetpos( handle, &current_chunk.location ) != 0 ) {
		return FALSE;
	}

	// 00 00 00 0D | 49 48 44 52 | ?? ?? ?? ?? | 12 A0 05 5F
	//     SIZE    |     NAME    |     DATA    |    CRC32
	// SIZE
	BYTE size_buffer[ 4 ];
	for ( unsigned char i = 0; i < 4; i++ ) {
		size_buffer[ 3 - i ] = fgetc( handle );
		if ( feof( handle ) ) {
			return FALSE;
		}
	}
	current_chunk.size = *( (int*)size_buffer );

	// NAME
	current_chunk.name = (char*)calloc( 5, sizeof( char ) );
	for ( unsigned char i = 0; i < 4; i++ ) {
		current_chunk.name[ i ] = (char)fgetc( handle );
		if ( feof( handle ) ) {
			return FALSE;
		}
	}

	// DATA
	if ( current_chunk.size > max_length && max_length != 0 ) {
		current_chunk.data = (BYTE*)nullptr; // Ignore it.
		if ( fseek( handle, current_chunk.size, SEEK_CUR ) != 0x0 ) {
			free( current_chunk.data );
			free( current_chunk.name );
			return FALSE;
		}
	}
	else {
		current_chunk.data = calloc( current_chunk.size, sizeof(BYTE) );
		for ( unsigned int i = 0; i < current_chunk.size; i++ ) {
			current_chunk.data[ i ] = fgetc( handle );
			if ( feof( handle ) ) {
				free( current_chunk.data );
			free( current_chunk.name );
				return FALSE;
			}
		}
	}

	// CRC32
	for ( unsigned char i = 0; i < 4; i++ ) {
		current_chunk.checksum += (int)fgetc( handle );
		if ( feof( handle ) ) {
			return FALSE;
		}
	}

	*buffer = current_chunk;
	return TRUE;
}

BOOL parse_ihdr( BYTE* data, ihdr_data* output_buffer ) {
	ihdr_data ihdr_output = { .width = 0, .height = 0, .bit_depth = 1,
		.colour_type = 1, .compression_type = 1, .filter_type = 1,
		.interlace_type = 1 };

	//  4b    +  4b  = [8b]/64B
	// Height & width:
	for ( unsigned char i = 0; i < 4; i++ ) {
		( (BYTE*)(&ihdr_output.width)) [ 3 - i ] = data[ i ];
		( (BYTE*)(&ihdr_output.height)) [ 3 - i ] = data[ i + 4 ];
		// w0 w0 w0 w0 | h0 h0 h0 h0
	}

	// 1b + [8b] = [9b]
	// Bit-depth:
	ihdr_output.bit_depth = data[ 8 ];

	// 1b + [9b] = [10b]
	// Colour-type:
	ihdr_output.colour_type = data[ 9 ];

	// 1b + [10b] = [11b]
	// Compression-type:
	ihdr_output.colour_type = data[ 10 ];

	// 1b + [11b] = [12b]
	// Filter-type:
	ihdr_output.colour_type = data[ 11 ];

	// 1b + [12b] = [13b] -> TOTAL
	// Interlace-type:
	ihdr_output.colour_type = data[ 12 ];

	*output_buffer = ihdr_output;
	return TRUE;
}

BOOL list_ascillary_full( FILE* png_handle ) {
	chunk iterative_chunk;
	ihdr_data ihdr;
	while ( read_chunk( png_handle, 1000, &iterative_chunk ) ) {
		printf( "%s\n|_|\n |\n |--- Location: 0x%08X\n |--- Size: 0x%08X\n |--- CRC32: 0x%08X\n\n",
			iterative_chunk.name, (unsigned int)iterative_chunk.location.__pos,
			iterative_chunk.size, iterative_chunk.checksum );
		if ( strncmp( iterative_chunk.name, "IHDR", 4 ) != 0 ) {
			continue;
		}
		if ( !parse_ihdr( iterative_chunk.data, &ihdr ) ) {
			printf( "Unable to parse IHDR\n" );
			return FALSE;
		}
	}
	printf( "\nFile summary:\n\tResolution: %d x %d\n\tBit-depth: %d\n\tColour-type: %d\n\n",
		ihdr.width, ihdr.height, ihdr.bit_depth, ihdr.colour_type );
	return TRUE;
}

BOOL strip_chunk( FILE* png_handle, const char* chunk_name ) {
	chunk iterative_chunk;
	while ( read_chunk( png_handle, 1000, &iterative_chunk ) ) {
		if ( strncmp( iterative_chunk.name, chunk_name, strlen( chunk_name ) ) != 0 ) {
			continue;
		}

		// Found the chunk!		
		if ( 0 != fseek( png_handle, iterative_chunk.location.__pos + 4, SEEK_SET ) ) {
			printf( "Unable to perform an IO operation while wiping chunk '%s'.\n",
				iterative_chunk.name );
			continue;
		}

		// Adding eight to iterative_chunk.size allows us to
		// overwrite the CRC32 which could provide information
		// to anyone that needs to narrow down the possible
		// value(s) of the chunk we are wiping.
		// That only accounts for four bytes though (CRC32 =
		// four byte integer in PNG) - the other four bytes
		// are the chunk's identifier (four 1-byte characters)
		// so that anyone analyzing the PNG also can't tell
		// what chunk was previously there.
		for ( unsigned short byte_index = 0; byte_index < iterative_chunk.size + 8; byte_index++ ) {
			if ( 0 != fputc( (char)0x0, png_handle ) ) {
				printf( "Unable to write to chunk '%s'.\n", iterative_chunk.name );
			}
			if ( feof( png_handle ) ) {
				printf( "Unable to write at 0x%08X (chunk '%s').",
					(unsigned int)( iterative_chunk.location.__pos + byte_index ),
					iterative_chunk.name );
				return FALSE;
			}
		}

		printf( "Filled '%s' with null bytes.\n", iterative_chunk.name );
		return TRUE;
	}

	printf( "Unable to locate chunk '%s' within the provided file.\n", iterative_chunk.name );
	return FALSE; // We couldn't find that chunk.
}

int main( int argc, char** argv ) {

	if ( argc != 2 && argc != 3 ) {
		printf( "Invalid amount of arguments provided.\n" );

		printf( "Usage:\n\t%s [file.png] | List the chunks in 'file.png'.\n\t%s [file.png] [chnk] | Erases chunk 'chnk' in 'file.png'.\n",
			argv[ 0 ], argv[ 0 ] );

		return -1;
	}

	FILE* png_handle = fopen( argv[ 1 ], "r+" );
	if ( !png_handle ) {
		printf( "Unable to open a handle to file '%s'.\n", argv[ 1 ] );
		return -1;
	}

	// The first bytes of a PNG should be 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x1A, 0x0A
	BYTE* magic_bytes_buffer = (BYTE*)malloc( sizeof(BYTE) * ( 7 + 1 ) );
	if ( !read_bytes( png_handle, 8, magic_bytes_buffer ) ) {
		fclose( png_handle );
		free( magic_bytes_buffer );
		printf( "Unable to read the first seven bytes of '%s'.\n", argv[ 1 ] );
		return -1;
	}
	// I'm not sure why but the last two bytes need to be reversed in order (0x0A <-> 0x1A)
	// this shouldn't impact consistency as far as I'm aware.
	if ( strncmp( magic_bytes_buffer, "\x89\x50\x4E\x47\x0D\x0A\x1A",
		(long unsigned int)7 ) != 0 ) {
		printf( "Unable to verify that the provided file ('%s') is a PNG.\n", argv[1] );
		fclose( png_handle );
		free( magic_bytes_buffer );
		return -1;
	}
	free( magic_bytes_buffer );
	printf( "Validated PNG magic bytes.\n" );

	if ( argc == 2 ) {
		list_ascillary_full( png_handle );		
	}
	else {
		strip_chunk( png_handle, argv[ 2 ] );
	}

	fclose( png_handle );
	return 1;
}