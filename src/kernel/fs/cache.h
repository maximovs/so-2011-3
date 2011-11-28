// The following calls are used by our filesystem to write to the disk.
#include "../../drivers/atadisk.h"
#define SECTORS 500
#define SECTOR_SIZE 512
#define TRUE 1
#define FALSE 0


int _cache_init(void);


int _cache_read(int ata, char * ans, int numreads, unsigned int sector);


int _cache_write(int ata, char * msg, int numreads, unsigned int sector);


int _cache_flush(void);