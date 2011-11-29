// #include "smdint.h"
// #include  "interrupts.h"
// #include "file_descriptors.h"
#include "../../../include/defs.h"
// #include "page.c"
/* Number of bits in a storage unit */
#define CHAR_BIT 8

void init_paging( void );


uint32_t create_proc_ptable( void );

void clear_proc_ptable( uint32_t offset );

uint32_t get_stack_start( uint32_t pdir_offset);

void* memset( void* buffer, int ch, size_t count );

void takedown_user_page(void* addr, int perms, int flag);
