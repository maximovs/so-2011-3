#include "page.h"
// #include "tagfs_queries.h"
// #include "file_descriptors.h"
// #include "smdlib.h"
// #include "file.h"
// #include "smdio.h"
// #include "smdstring.h"
// #include "smdfile.h"

#define KERNEL_MEM 0x10000000
#define TOTAL_KERNEL_MEM 0x10000000

//Start of the physical pages in memory (slots)

#define USER_MEM_START (TOTAL_KERNEL_MEM)

//Start of the all process' virtual memory

#define USER_VIRTUAL_MEM_START (TOTAL_KERNEL_MEM)

#define PAGE_SIZE 4096

#define P_DIR_START 0x08000000

#define P_TABLE_START (P_DIR_START + PAGE_SIZE)
#define PTABLE_ENTRIES 1024

#define KERNEL_PAGE_TABLES   ( KERNEL_MEM >> 22)

#define TOTAL_KERNEL_PAGE_TABLES (TOTAL_KERNEL_MEM >> 22)

#define P_TABLE_USER_START (P_TABLE_START + TOTAL_KERNEL_PAGE_TABLES * PAGE_SIZE)

#define RWUPRESENT 7

#define MAX_PROC 128

#define RWUNPRESENT 6

#define PAGES_ON_MEM 12

#define USER_PTABLE_OFFSET ((P_TABLE_USER_START - P_TABLE_START)/4)



typedef uint32_t ptable_entry;

typedef uint32_t pdir_entry;



//Simple Vector which tells us which process page table is used or free
//0 free, 1 used

static uint32_t dirs[128] = {0};

//Vector which tells which page slots are used and by whom
// -1 unused
//on page[k] we save the offset of the entry from P_TABLE_USER_START
//which is using de kth page slot. Since all processes table are continous in memory
//when a process gets a page fault and wants the kth slot, the page table entry of the process
//which used this slot can be easily retreived (it's just (ptable_entry)P_TABLE_USER_START + pages[k])

static uint32_t pages[PAGES_ON_MEM] = {0};

//Round Robin Offset

static uint32_t p_off = 0;


void* memset( void* buffer, int ch, size_t count ) {

	unsigned int mold = 0;
	size_t i;
	for ( i = 0; i < sizeof(unsigned int); i++ )
		mold = (mold << CHAR_BIT) | (unsigned int)(unsigned char)ch;

	unsigned int* buf = buffer;
	size_t sz = count / sizeof(unsigned int);

	for( ; sz; count -= sizeof(unsigned int), sz-- )
		*buf++ = mold;

	unsigned char* buf2 = (unsigned char*)buf;
	for ( ; count; count-- )
		*buf2++ = (unsigned char)ch;

	return buffer;
}

static pdir_entry get_dir_entry(  uint32_t address, uint32_t perms){
  
     ptable_entry ret = address & 0xFFFFF000;
     ret |= perms;
     return ret;
}

static ptable_entry get_table_entry(  uint32_t address, uint32_t perms){
  
     pdir_entry ret = address & 0xFFFFF000;
     ret |= perms;
     return ret;
}

inline static uint32_t get_dir_entry_add( pdir_entry entry){
  
    return entry & 0xFFFFF000;
}

inline static uint32_t get_table_entry_add( ptable_entry entry){
  
    return entry & 0xFFFFF000;
}

/**
 * Constructs a virtual address associated with a certain page_table_entry
 * and returns it aligned to page.
 * @param ptable_offset 	offset of said page_table_entry from USER_PTABLE_START
 * 
 * @return 			virtual memory aligned to page
 **/

static uint32_t construct_address(uint32_t ptable_offset){
  
    uint32_t val  = ptable_offset + USER_PTABLE_OFFSET;
    uint32_t top = val >> 10;
    uint32_t middle = val & 0x000003FF;
    return (top << 22) + (middle << 12);
}

#define READ_PAGE true
#define WRITE_PAGE false


/**
 * Should write the memory of the page in disk, and also save
 * in the page table register some data to be able to access it later
 * the ptable entry can be accessed by :
 *     (ptable_entry *)P_TABLE_USER_START + ptable_offset
 * Should set the last bit to 0 obviously, after writing it down, since we read from
 * the virtual address to access the content of the page.
 * @param ptable_offset  Total offset of the entry which is using the slot we want
 * 			from P_TABLE_USER_START. We save the virtual address aligned to page
 * 			that would cause a page_fault because of this page absence in the ptable entry
 * 			The file is named after said address
 */
static void page_down( uint32_t ptable_offset){
  
	if (ptable_offset == 0xFFFFFFFF) {
		return;
	}

	uint32_t address = construct_address(ptable_offset);
	ptable_entry *entry = (ptable_entry*)P_TABLE_USER_START + ptable_offset;

	address &= ~0xfff;

	*entry = get_table_entry(address, RWUNPRESENT);

	return;
}

/**
 * Should bring the page referenced by entry to memory.
 * The memory should be copied to:
 *     USER_MEM_START + page_offset * PAGE_SIZE
 * The data should be found in disk. entry contains a 0 in the end
 * and the rest should be used to access to disk (page_down sets these bits)
 * We set the page as present before writing since we use the virtual address mapped to the page
 * to write on it.
 * 
 * 
 * @param page_offset 	Offset of the physical page to use (which page slot we are going to use)
 * 			The slot should be empty since page_down is always called before page_up
 * @param entry		unpresent pagetable entry which caused the fault. page_down previously saved
 * 			the virtual address which would cause this page_fault so it is easy to find the file.
 */
static void page_up( uint32_t page_offset, ptable_entry * entry) {
  
    uint32_t address = (uint32_t)get_table_entry_add(*entry);
    uint32_t target_address = USER_MEM_START + page_offset * PAGE_SIZE;
    *entry = get_table_entry(target_address, RWUPRESENT);

	address &= ~0xfff;
    return;
}

void page_fault_handler( uint32_t errcode, uint32_t address) {

	pdir_entry * dir = ((pdir_entry *)P_DIR_START) + (address >> 22);
	uint32_t ptable_off = address >> 12;
	ptable_off = ptable_off & 0x000003FF;

	
	ptable_entry *ent = (ptable_entry*)get_dir_entry_add(*dir);
	ent += ptable_off;

	if ((*dir & 0x1) == 0) {
		//kernel panic... page tables are always in memory!!
		while (1);
	}
	uint32_t add = USER_VIRTUAL_MEM_START + PAGE_SIZE * p_off;

	if (*ent == 0){

		page_down(pages[p_off]);
		*ent = get_table_entry( add, RWUPRESENT);

		// not necessary. However the page is another process' RAM, and
		// processes should have other process' looking at their ram
		memset((void *)(address&(~0xfff)), 0, PAGE_SIZE);
	} else {

		//search in disk for initialized page
		page_down(pages[p_off]);
		page_up( p_off, ent );
	}
	
	//Round Robin
	
	uint32_t off = ent - (ptable_entry *) P_TABLE_USER_START;
	pages[p_off] = off; 
	p_off++;
	if (p_off == PAGES_ON_MEM) {
		p_off = 0;
	}

	return;
}


// void page_fault_handler_wrapper( struct int_params* params){
//   
//     uint32_t address = 0;
//     __asm__ volatile("MOVL 	%%CR2, %0" : "=r" (address) : );
//     page_fault_handler( params->err_code, address);
//   
// }

void clear_proc_ptable( uint32_t offset) {
  
    dirs[offset] = 0;
}

static void set_proc_ptable( uint32_t offset ) {

	pdir_entry *dir = (pdir_entry *)P_DIR_START;
	
	//Get the virtual address for the process
	uint32_t mem = USER_VIRTUAL_MEM_START + PAGE_SIZE * 1024 * offset;
	dir += (mem >> 22);
	//Adress of the start of the process' page table
	ptable_entry * table = (ptable_entry  *) (P_TABLE_USER_START + offset * PAGE_SIZE);
	
	//Fill the directory entry for the process' page table
	*dir = get_dir_entry( (uint32_t) table, RWUPRESENT);
	int i = 0;

	//Sets all page table entries as not present, not initialized
	for( i = 0 ; i < PTABLE_ENTRIES ; i ++ ) {
		table[i] = 0;
	}
}

uint32_t get_stack_start( uint32_t pdir_offset ) {
  
    return USER_VIRTUAL_MEM_START + pdir_offset * PAGE_SIZE * PTABLE_ENTRIES;
}

uint32_t create_proc_ptable( void ) {
  
    int i = 0;
    for ( i = 0 ; i < MAX_PROC ; i ++ ) {
		if(dirs[i] == 0) {
			set_proc_ptable( i );
			dirs[i] = 1;
			return i;
		}
	}
    return -1;
}

static void test_page_consistency( uint32_t random_dir) {
    
    uint32_t dir_offset = random_dir >> 22;
    uint32_t ptable_offset = (random_dir >> 12) & 0x3FF;
    pdir_entry *entry = (pdir_entry *)P_DIR_START + dir_offset;

    ptable_entry *tbl = ((ptable_entry *)get_dir_entry_add(*entry)) + ptable_offset;
    uint32_t final = get_table_entry_add( *tbl );

    if( final >> 12!= random_dir >> 12){
		printf("DETECTED INCONSISTENCY\n");
		// Kernel panic
		while(1);
    }
}

/**
 * Creates a kernel page, one to one mapping.
 * Kernel priviligies.
 * Assumes corresponding page table is created
 * 
 * @param addr		address which wants to be mapped
 **/
static void create_kernel_page( void* addr) {
  
      uint32_t pdir_offset = ((uint32_t) addr) >> 22;
      pdir_entry *dir = (pdir_entry *)P_DIR_START + pdir_offset;
      ptable_entry *tab = (ptable_entry * )get_dir_entry_add( *dir );
      ptable_entry *entry = tab + ((((uint32_t) addr)>> 12) & 0x3FF);
      *entry = get_table_entry( (uint32_t ) addr, RWUPRESENT );
}

/**
 * Creates a kernel page table, one to one mapping
 * Kernel priviligies.
 * 
 * @param addr		address which wants to be mapped
 * @param off		pagetable offset
 **/
static void create_kernel_ptable(  void* addr ) {
  
      uint32_t pdir_offset = ((uint32_t) addr) >> 22;
      pdir_entry *dir = (pdir_entry *)P_DIR_START + pdir_offset;
      *dir = get_dir_entry(P_TABLE_START + pdir_offset * PAGE_SIZE, RWUPRESENT );
}

/**
 * Initializes kernel pages.
 * One to one mapping
 **/
static void init_kernel_pages( void ){
	int i,j;
    for ( i = 0 ; i < TOTAL_KERNEL_PAGE_TABLES ; i ++ ) {
		void *addr = (void *)(i * PTABLE_ENTRIES * PAGE_SIZE);
		create_kernel_ptable(addr);
		for( j = 0; j < PTABLE_ENTRIES ; j ++) {
			create_kernel_page((void*)((uint32_t)addr + j * PAGE_SIZE));
		}
    }
}

static ptable_entry down_table_entry(uint32_t address, uint32_t perms, int flag) {

	pdir_entry ret = address & 0xFFFFFFF1;
	ret |= perms;
	return ret;
}

void takedown_user_page(void* addr, int perms, int flag) {

	uint32_t pdir_offset = ((uint32_t) addr) >> 22;/*va desde 0 a 63, para ver donde comienza la tabla en cuestion*/
	pdir_entry *dir = (pdir_entry *) P_DIR_START + pdir_offset; /*busca la tabla con el offset anterior*/
	ptable_entry *tab = (ptable_entry *) get_dir_entry_add(*dir); /*se queda con los 20 bits mas significativos*/
	ptable_entry *entry = tab + ((((uint32_t) addr) >> 12) & 0x3FF); /*se queda con los 10 bits menos significativos
	 de addr*/
	*entry = down_table_entry((uint32_t) addr, perms, flag); /*setea el page frame address de la pagina, poniendola
	 como presente*/
}

/**
 * Enables hardware paging.
 * Kernel pages MUST have been initialized beforehand
 **/
static void enable_paging( void ) {
  
	uint32_t directory = P_DIR_START;
      
	//Set the page_directory address in CR3
	__asm__ volatile("MOVL 	%0, %%CR3" : : "b" (directory) : "%eax");
	__asm__ volatile("MOVL	%CR0, %EAX"); 			// Get the value of CR0.
	__asm__ volatile("OR	$0x80000000, %EAX");	// Set PG bit.
	__asm__ volatile("MOVL 	%EAX, %CR0");			// Set CR0 value.
}

void init_paging( void ) {
	int i;
    init_kernel_pages();

    // register_interrupt_handler ( 14, page_fault_handler_wrapper );

    for(i = 0 ; i < PAGES_ON_MEM ; i ++ ) {
		pages[i] = -1;
    }
    enable_paging();
}

