#ifndef TWO_LEVEL_PREFETCHER
#define TWO_LEVEL_PREFETCHER

#include "Prefetcher.h"
#include <stdbool.h>

typedef struct entry
{
	unsigned int region;
	int age;
	bool valid;
}entry_t;

typedef struct tracker
{
	unsigned int pc;
	unsigned int last_offset;
	int stride;
	int age;
	bool valid;
}tracker_t;


class TwoLevelPrefetcher : public Prefetcher
{
	private:
		bool debug;
		char name[100];

		entry_t *region_map;
		tracker_t *trackers;
		unsigned int region_map_size;
		unsigned int trackers_size;
		unsigned int trackers_per_region;

		unsigned int stat_total_prefetch;
		unsigned int stat_total_region_map_repl;
		unsigned int stat_total_trackers_repl;

	public:
		TwoLevelPrefetcher(){}
		~TwoLevelPrefetcher(){}
		void prefetcher_init(char*, bool, int);
		void prefetcher_init(char*, bool, unsigned int, unsigned int);
		int  prefetcher_operate(unsigned int, unsigned int, unsigned int*);
		void prefetcher_heartbeat_stats();
		void prefetcher_final_stats();
		void prefetcher_destroy();

};

#endif /* TWO_LEVEL_PREFETCHER */

