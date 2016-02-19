#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ASSERT(cond,msg) do{if(!(cond)){fprintf(stderr,msg);exit(1);}}while(0)

//To be called for initializing prefetcher
void prefetcher_init(bool debug);

//Returns 1 if it prefetches, and assignes the address to be prefetched in prefAddr
//Returns -1 otherwise
int prefetcher_operate(unsigned int ip, unsigned int addr, unsigned int *prefAddr);

void prefetcher_heartbeat_stats();
void prefetcher_final_stats();

//deallocates everything
void prefetcher_destroy();

#endif

