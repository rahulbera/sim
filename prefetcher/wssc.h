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

typedef struct plt
{
	uint tag;
	bool valid;
}plt_t;

typedef struct sat
{
	unsigned long int pht_tag;
	bool *pattern;
	int age;
	bool valid;
}sat_t;

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
		float threshold;
		plt_t **plt;
		sat_t **sat;
		uint plt_size, plt_assoc, plt_sets, plt_sets_in_log;
		uint sat_size, sat_assoc, sat_sets, sat_sets_in_log;
		ulong interval;
		SMSPrefetcher *prefetcher;

		/* Stat variables */
		uint total_invalidation;
		uint total_access;
		uint total_insert;
		uint replacementNeeded;
		uint samePHTAccess;
		uint diffPHTAccess;
		std::unordered_map<uint, counter_t*>pc_prefetch_map;

	public:
		wssc();
		~wssc();
		void init(char*, uint, uint, uint, ulong, float);
		void link_prefetcher(SMSPrefetcher*);
		ulong get_interval();
		float get_threshold();
		bool plt_find(uint,uint*,uint*,uint*);
		bool insert(uint, ulong, bool*, uint);
		void update(uint, uint);
		void invalidate();
		void heart_beat_stats();
		void final_stats();

		/* debug functions */
		void debug_sat_entry(uint, uint);
		void dump(uint);
	
	private:
		uint log2(uint);
};

#endif /* WSSC_H */

