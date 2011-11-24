#ifndef _MEM_H_
#define _MEM_H_
#include "../../../include/defs.h"

// #include "klib.h"

/* Pages of 4KB */
#define PAGESIZE 4096
#define NENTRIES 1024

typedef unsigned int pentry_t;
typedef unsigned int address_t;
typedef pentry_t * ptbl_t;

#define NPAGES 1048576
#define BITMAP_BYTESIZE (NPAGES / 8)
#define BITMAP_PAGESIZE (BITMAP_BYTESIZE / PAGESIZE)

/* First 8 MB are reserved for Kernel and Page allocation */
#define RESERVED_MEM ((address_t) 0x800000)

/* The firsts 4MB are for the kernel, the page directory and the kernel table.*/
/* The last page of the first 4MB will be the page directory */
#define KERNEL_AREA (RESERVED_MEM - ((address_t) 0x400000))

#define SETBIT(X, P, V) (V ? ((1 << (P)) | (X)) : (~(1 << (P)) & X))

/* PRESENT, READ/WRITE AND SUPERVISOR BIT */
#define P_RW_SV 0x07
/* NOT PRESENT, READ/WRITE AND SUPERVISOR BIT */
#define NP_RW_SV 0x06

#define ISPRESENT(X) ((X) & 0x01)
#define PRESENT 0x01

#define GETDIRENTRY(X) (((X) & 0xFFC00000)>>22)
#define GETTBLENTRY(X) (((X) & 0x003FF000)>>12)
#define GETPGOFFSET(X) ( (X) & 0x00000FFF)

#define GETADDRESS(X) ((X) & 0xFFFFF000)

enum {
	NO_ERRORS,
	ERROR_ILLEGALPAGE
};

/* Creates the page directory with the firsts 4MB using Identity Mapping.
 * Fill the Page directory with the Kernel Table.
 * And sets the CR3 registry.
 * */
void _startPaging(int kbytes);

struct block_t {
	struct block_t * next;
	void * firstPage;
	unsigned int npages;
	unsigned int freeSpace;
	void * ptrFreeMemory;
};

typedef struct block_t * block_t;

int _pageUp(void * pg);
int _pageDown(void * pg);
void* _sys_malloc(size_t size);
void _sys_free(void *pointer, int npages);

/* The following functions take a pointer to a Page Directory Entry, and fill
*   the structure with the given information 
*   */
pentry_t 	_pd_createEntry(void* address);
void        _pd_togglePresent(pentry_t*);

// void* _reqpage(task_t task);

// void* malloc(size_t size);
// void free(void *pointer);

struct PagingNamespace {
	void (*start)(int);
	int (*pageUp)(void * pg);
	int (*pageDown)(void * pg);
	void (*freeMem)(void *, int );
};

// extern struct system_t System;

#endif
