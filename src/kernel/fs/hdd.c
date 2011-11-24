/*
 *  hdd.c
 *  SO-FS
 *
 *  Created by Cristian Pereyra on 19/10/11.
 *  Copyright 2011 My Own. All rights reserved.
 *
 */

// Made for debugging purposes, used as RAM on OSX, and as a wrapper in real life, probably used for cache later on.


#include "hdd.h"
#include "fs.h"

static int cache;

void hdd_init() {
	// Nothing to do on this end.
	cache = 0;
}

void hdd_read(char * answer, unsigned int sector) {
	if(cache){
		_cache_read(ATA0, answer, 2, sector);
	}
	else{
		_disk_read(ATA0, answer, 2, sector);
	}
}

void hdd_write(char * buffer, unsigned int sector) {
	if(cache){
		_cache_write(ATA0, buffer, 2, sector);
	}
	else{
		_disk_write(ATA0, buffer, 2, sector);
	}
}

void hdd_close() {
	// Nothing to do on this end.
}

void hdd_goCacheless(){
	cache = 0;
}

void hdd_goCache(){
	cache = 1;
}