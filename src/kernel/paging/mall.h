/**
 * @file smdlib.h
 *
 * ANSI C standard library
 */

#ifndef _SMDLIB_H_
#define _SMDLIB_H_

#include "../../../include/defs.h"

// #include "smddef.h"
// #include "smdtype.h"


/**
 * Initializes the memory and bins arrays.
 *
 * Must be called before any malloc or free.
 *
 */
void init_malloc ( void );


/**
 * Allocs a chunk of memory of a given size.
 *
 * @param size  the size trying to be allocated (in bytes)
 *
 * @return a pointer to the allocated memory, or NULL if an error occurred
 *
 * For more info on the algorithm idea:
 * @see http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 */
void* e_malloc ( size_t size );


/**
 * Frees the previously allocated space.
 *
 * @param data  a pointer to the memory to be freed
 *
 */
void e_free ( void* date );

/**
 * Stub realloc
 *
 */
void* e_realloc(void* ptr, size_t new_size);

/**
 * Print memory data to screen.
 * Intended for debugging.
 *
 */
void print_memory_data ( void );


/**
 * Absolute value of an int
 *
 * @param n  the number
 *
 * @return the absolute value of n
 *
 */
static inline int abs( int n ) {

	return ( n < 0 ) ? -n : n;
}


/**
 * Absolute value of a long
 *
 * @param n  the number
 *
 * @return the absolute value of n
 *
 */
static inline long labs( long n ) {

	return ( n < 0 ) ? -n : n;
}


/**
 * Absolute value of a long long
 *
 * @param n  the number
 *
 * @return the absolute value of n
 *
 */
static inline long long llabs( long long n ) {

	return ( n < 0 ) ? -n : n;
}


static inline int atoi( const char *nptr ) {

	int ret = 0;
	while ( isdigit( *nptr ) )
		ret = 10*ret + *(nptr++) - '0';

	return ret;
}


static inline long atol( const char *nptr ) {

	long ret = 0;
	while ( isdigit( *nptr ) )
		ret = 10*ret + *(nptr++) - '0';

	return ret;
}


static inline long long atoll( const char *nptr ) {

	long long ret = 0;
	while ( isdigit( *nptr ) )
		ret = 10*ret + *(nptr++);

	return ret;
}

static inline int itoa ( char* buf, unsigned int value, int base ) {

	int wrote = 0;
	unsigned int mod;
	while (wrote < 9 && value) {
		mod = value % base;
		buf[wrote++] = mod < 10 ? mod + '0' : mod + 'a' - 10;
		value /= base;
	}

	return wrote;
}


#endif /* _MEMORY_H_ */
