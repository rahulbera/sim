#ifndef WSSC_H
#define WSSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SMSPrefetcher.h"
#define ASSERT(cond,msg) do{if(!(cond)){fprintf(stderr,msg);exit(1);}}while(0)

typedef unsigned int uint;
typedef unsigned long int ulong;

typedef struct wssc_entry
{
	uint tag;
	ulong pht_tag;
	bool *pattern;
	bool valid;
}wssc_t;

class SMSPrefetcher;

class wssc
{
	private:
		char name[100];
		wssc_t **map;
		uint size;
		uint associativity;
		uint sets;
		uint setsInLog;
		ulong interval;
		SMSPrefetcher *prefetcher;

		/* Stat variables */
		uint total_invalidation;

	public:
		wssc();
		~wssc();
		void init(char*, uint, uint, ulong);
		void link_prefetcher(SMSPrefetcher*);
		ulong get_interval();
		bool find(uint,uint*,uint*,uint*);
		void insert(uint, ulong, bool*, uint);
		void update(uint, uint);
		void invalidate();
		void heart_beat_stats();
		void final_stats();

		/* debug functions */
		void debug_wssc_entry(uint, uint);
		void dump(uint);
	
	private:
		uint log2(uint);
};

#endif /* WSSC_H */

