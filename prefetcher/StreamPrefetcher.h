#ifndef STREAMPREFETCHER_H
#define STREAMPREFETCHER_H

#include <map>
#include "Prefetcher.h"

/* Line size: 64B, Page: 4KB */
#define LINE_SIZE_LOG 6 
#define PAGE_SIZE_LOG 12

typedef struct entry
{
	unsigned int page;
	unsigned int last_line;
	int stride;
	int age;
	bool valid;
}tracker_t;

typedef struct info
{
	unsigned int last_line;
	unsigned int prefetch_line;
}info_t;


class StreamPrefetcher : public Prefetcher
{
	private:
		char name[100];
		tracker_t *trackers;
		int size;
		unsigned int last_accessed_line;
		unsigned int last_prefetched_line;
		std::map<unsigned int, info_t*> prefetch_map;
		std::map<unsigned int, info_t*>::iterator it;

		/* Stats variables */
		unsigned int stat_total_mem_access;
		unsigned int stat_total_prefetch;
		unsigned int stat_total_prefetch_hit2;
		unsigned int stat_heart_beat_prefetch;
		unsigned int stat_heart_beat_prefetch_hit2;

	public:
		StreamPrefetcher(){}
		~StreamPrefetcher(){}
		void prefetcher_init(char *name, int n);
		int  prefetcher_operate(unsigned int ip, unsigned int addr, unsigned int *prefAddr);
		void prefetcher_heartbeat_stats();
		void prefetcher_final_stats();
		void prefetcher_destroy();

};

#endif /*STREAMPREFETCHER_H*/


