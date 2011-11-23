// The following calls are used by our filesystem to write to the disk.
#include "../../drivers/atadisk.h"
#define SECTORS 20
#define SECTOR_SIZE 512
#define TRUE 1
#define FALSE 0
/*
	_cache_init
	Initializes de cache

*/
int _cache_init(void);
/*
	INTERRUPTS MUST BE DISABLED BEFORE USING THIS CALL

	 _disk_read
	ata: ATA controller to use, but actually deprecated to mantain compatibility.
	ans: Buffer to read in.
	numreads: Number of sectors to read.
	sector: Sector to start reading in.
*/
int _cache_read(int ata, char * ans, int numreads, unsigned int sector);


/*
	INTERRUPTS MUST BE DISABLED BEFORE USING THIS CALL

	 _disk_write
	ata: ATA controller to use, but actually deprecated to mantain compatibility.
	ans: Buffer to write in.
	numreads: Number of sectors to write.
	sector: Sector to start writing in.
	
*/
int _cache_write(int ata, char * msg, int numreads, unsigned int sector);



int _cache_flush(void);