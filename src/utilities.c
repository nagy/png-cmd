#include "png-cmd.h"
#include <ctype.h>

void free_chunk( pchunk chnk ) {
	if ( chnk->data != nullptr ) {
		free( chnk->data );
		chnk->data = nullptr;
	}
}

bool is_string_number( const char* string, size_t len ) {
	for ( size_t i = 0; i < len; i++ ) {
		if ( !isdigit( string[ i ] ) ) {
			return false;
		}
	}
	return true;
}

// Assuming that the caller has already validated that the
// buffer is large enough to facilitate this and also that
// the file in question is long enough to read this many bytes.
bool read_bytes( FILE* file_handle, size_t len, BYTE* buffer ) {
	if ( buffer == nullptr || len == 0 ) {
		return false;
	}
	for ( size_t i = 0; i < len; i++ ) {
		BYTE iterative_byte = fgetc( file_handle );
		if ( feof( file_handle ) ) {
			return false;
		}
		buffer[ i ] = iterative_byte;
	}
	return true;
}

uint8_t get_number_length( int64_t number ) {
    uint8_t length;
    for ( length = number < 0 ? 2 : 1; number != 0 && length != 255;
		number /= 10, length++ ) { }
	return length;
}
