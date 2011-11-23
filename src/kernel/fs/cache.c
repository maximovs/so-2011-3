#include "cache.h"

typedef struct sectors {
	
	unsigned int first_sector;
	char sectors[SECTORS*SECTOR_SIZE];
	int dirty;
	
	} sectors;

	static sectors cache_handler;

int _cache_init(){
	int i = 0;
	for( i=0; i<SECTORS*SECTOR_SIZE; i++ )
		cache_handler.sectors[i] = 0;
	cache_handler.dirty = FALSE;
	cache_handler.first_sector = 0;
	return 0;
}

int _cache_read(int ata, char * ans, int numreads, unsigned int sector){
	int i = 0;
	if( sector < cache_handler.first_sector || sector + numreads >= cache_handler.first_sector + SECTORS ){
		if( cache_handler.dirty )
			_cache_flush();
		for( i=0; i<SECTORS/2; i++)	
			_disk_read(ata, cache_handler.sectors + i*2*SECTOR_SIZE, 2, sector+i*2);
		cache_handler.first_sector = sector;
		if(numreads < SECTORS ){
			for( i=0; i<numreads*SECTOR_SIZE; i++){
				ans[i] = cache_handler.sectors[i];
			}
			return 0;
		}
		else{
			return _disk_read(ata, ans, numreads, sector);
		}
	}
	else{
		int j = 0;
		for( i=(sector-cache_handler.first_sector)*SECTOR_SIZE; i<numreads*SECTOR_SIZE; i++ ){
			ans[j++] = cache_handler.sectors[i];
		}
	}
	return 0;
	
}

int _cache_write(int ata, char * msg, int numreads, unsigned int sector){
	if( sector < cache_handler.first_sector || sector + numreads > cache_handler.first_sector + SECTORS ){
		_cache_flush();
		
		return _disk_write(ata, msg, numreads, sector);
	}
	else{
		int i = (sector-cache_handler.first_sector)*SECTOR_SIZE;
		int j = 0;
		for( ; i<numreads*SECTOR_SIZE; i++ ){
			cache_handler.sectors[i] = msg[j++];
		}
		cache_handler.dirty = TRUE;
	}
	return TRUE;
}


int _cache_flush(void){
	cache_handler.dirty = FALSE;
	int i = 0;
	for( i=0; i<SECTORS/2; i++ ){
		_disk_write(ATA0, cache_handler.sectors + SECTOR_SIZE*i*2, 2, cache_handler.first_sector+i*2);
	}
	return 1;
		
}