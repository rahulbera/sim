#ifndef WSSC_H
#define WSSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unordered_map>
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

typedef struct counter
{
	uint counter1;
	uint counter2;
}counter_t;

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
		uint total_wssc_access;
		uint total_insert;
		uint replacementNeeded;
		uint samePHTAccess;
		std::unordered_map<uint, counter_t*>pc_prefetch_map;

	public:
		wssc();
		~wssc();
		void init(char*, uint, uint, ulong);
		void link_prefetcher(SMSPrefetcher*);
		ulong get_interval();
		bool find(uint,uint*,uint*,uint*);
		bool insert(uint, ulong, bool*, uint);
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

