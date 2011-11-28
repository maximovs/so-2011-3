#include "cache.h"

typedef struct sector {
	
	unsigned int index;
	char data[SECTOR_SIZE];
	int used;
	int dirty;
	
	} sector;

	int count = 0;
	static sector sectors[SECTORS];

int _cache_init(){
	int i = 0;
	for( i=0; i<SECTORS; i++ ){
		sectors[i].used = 0;
	}
}

int _cache_read(int ata, char * ans, int numreads, unsigned int sector){
	int i = 0;
	int cached = 0;
	for( i=0; i<SECTORS; i++ ){
		if( sectors[i].used && sectors[i].index == sector ){
			cached = 1;
			int j = 0;
			for( j=0; j<SECTOR_SIZE; j++ ){
				ans[j] = sectors[i].data[j];
			}
		}
	}
	if( !cached ){
		i = getFree();
		sectors[i].index = sector;
		_disk_read( ata, sectors[i].data, 1, sector);
		sectors[i].used = 1;
		sectors[i].dirty = 0;
		int j = 0;
		for( j=0; j<SECTOR_SIZE; j++ ){
			ans[j] = sectors[i].data[j];
		}
	}
	if( numreads == 1 ){
		return 1;
	}
	
	return _cache_read( ata, ans + SECTOR_SIZE, numreads-1, sector+1);
}

int _cache_write(int ata, char * msg, int numreads, unsigned int sector){
	int i = 0;
	int cached = 0;
	for( i=0; i<SECTORS; i++ ){
		if( sectors[i].used && sectors[i].index == sector ){
			sectors[i].dirty = 1;
			cached = 1;
			int j = 0;
			for( j=0; j<SECTOR_SIZE; j++ ){
				sectors[i].data[j] = msg[j];
			}
		}
	}
	if( !cached ){
		i = getFree();
		sectors[i].index = sector;
		int j = 0;
		for( j=0; j<SECTOR_SIZE; j++ ){
			sectors[i].data[j] = msg[j];
		}
		_disk_write( ata, sectors[i].data, 1, sector);
		sectors[i].used = 1;
		sectors[i].dirty = 0;

	}
	if( numreads == 1 ){
		return 1;
	}
	
	return _cache_read( ata, msg + SECTOR_SIZE, numreads-1, sector+1);
}


int _cache_flush(void){
	int i = 0;
	for( i=0; i<SECTORS; i++ ){
		if( sectors[i].used  /*sectors[i].dirty*/ ){
			_disk_write( ATA0, sectors[i].data, 1, sectors[i].index );
		}
//		sectors[i].used = 0;
	}
	return 1;
}


int getFree(){
	int i = 0;
	for( i=0; i<SECTORS; i++ ){
		if(!sectors[i].used){
			return i;
		}
	}
	if( sectors[count%SECTORS].dirty ){
		_disk_write( ATA0, sectors[count%SECTORS].data, 1, sectors[count%SECTORS].index);
	}
	sectors[count%SECTORS].used = 0;
	return count++%SECTORS;
}