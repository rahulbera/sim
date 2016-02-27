#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <assert.h>

#define ASSERT(cond,msg) do{if(!(cond)){fprintf(stderr,msg);exit(1);}}while(0)


class Prefetcher
{
	public:
		//To be called for initializing prefetcher
		virtual void prefetcher_init(char *name, int n)=0;

		//Returns 1 if it prefetches, and assignes the address to be prefetched in prefAddr
		//Returns -1 otherwise
		virtual int prefetcher_operate(unsigned int ip, unsigned int addr, unsigned int *prefAddr)=0;

		virtual void prefetcher_heartbeat_stats()=0;
		virtual void prefetcher_final_stats()=0;

		//deallocates everything
		virtual void prefetcher_destroy()=0;
};

#endif

