#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>
#include <arpa/inet.h>

/* #define BYTE unsigned char */
typedef uint8_t BYTE;
#define FALSE false
// #define nullptr 0x0
#ifndef __APPLE__
#define FPOS_GETVAL(val) (val.__pos)
#else
#define FPOS_GETVAL(val) (val)
#endif
// INT32_FLIP is surrounded by brackets (and omits a semicolon)
// so that it can be used in an evaluative context in the future.
// "All integers that require more than one byte must be in network byte order"
// ~ http://www.libpng.org/pub/png/spec/1.2/PNG-DataRep.html
#define INT32_FLIP(variable) (variable = ntohl(variable)) 

typedef struct png_chunk {
	fpos_t location; // Offset in file, highly platform-dependent.
	uint32_t size; // Not including name or checksum - watch out for integer (o/u)flows.
	unsigned char name [ 5 ]; // Don't print without '%.4s' to avoid an OOB read.
	BYTE* data; // Allocated using calloc so free this when you're finished.
	uint32_t checksum; // CRC32.
} chunk, *pchunk;

typedef struct ihdr_data {
// | <--4--> | <--4--> | <---1---> | <----1----> | <----1----> | <---1---> | <---1---> |
// |  width  | height  | bit-depth | colour-type | compression |   filter  | interlace |
//      4    +    4    +     1     +      1      +      1      +     1     +     1     = 13 bytes.
	int32_t width;
	int32_t height;
	BYTE bit_depth;
	BYTE colour_type;
	BYTE compression_type;
	BYTE filter_type;
	BYTE interlace_type;
} ihdr_data, *pihdr_data;

static const char colour_type_descriptions[7][20] = {
	"Grayscale",
	"Invalid",
	"RGB",
	"Custom Palette",
	"Grayscale & Alpha",
	"Invalid",
	"RGBA"
};

// png_chunk.c
bool read_chunk( FILE* file_handle, size_t max_length,
	chunk* output_buffer );
bool strip_chunk( FILE* png_handle, const char* chunk_name,
	const int chunk_index );
bool dump_chunk( FILE* file_handle,
	unsigned long target_chunk_index );
void free_chunk( pchunk chnk );

// file_io.c
bool read_bytes( FILE* file_handle, size_t len, BYTE* buffer );

// main.c
uint8_t get_number_length( int64_t number );
