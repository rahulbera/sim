#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <map>
#include "prefetcher.h"

/* Line size: 64B, Page: 4KB */
#define LINE_SIZE_LOG 6 
#define PAGE_SIZE_LOG 12
#define SIZE 64

bool debug;

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

tracker_t trackers[SIZE];
unsigned int last_accessed_line;
unsigned int last_prefetched_line;
std::map<unsigned int, info_t*> prefetch_map;
std::map<unsigned int, info_t*>::iterator it;

/* Stats variables */
unsigned int stat_total_mem_access = 0;
unsigned int stat_total_prefetch = 0;
unsigned int stat_total_prefetch_hit = 0;
unsigned int stat_total_prefetch_hit2 = 0;
unsigned int stat_heart_beat_prefetch = 0;
unsigned int stat_heart_beat_prefetch_hit = 0;
unsigned int stat_heart_beat_prefetch_hit2 = 0;

void prefetcher_init(bool debug_mode)
{
	if(debug)fprintf(stderr, "Multi-stream prefetcher, tracker count: %d\n", SIZE);
	debug = debug_mode;
	for(int i=0;i<SIZE;++i)
		trackers[i].valid = false;
	last_prefetched_line = 0;
}

int prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{
	unsigned int page = (addr >> PAGE_SIZE_LOG);
	unsigned int line_offset = ((addr >> LINE_SIZE_LOG) & 63);
	unsigned int line = (addr >> LINE_SIZE_LOG);
	
	stat_total_mem_access++;
	
	if(debug) fprintf(stderr, "P:%x L:%x LO:%d\n", page, line, line_offset);
	if(debug) fprintf(stderr, "LAL:%x LPL:%x\n", last_accessed_line, last_prefetched_line);

	/*	If it's accessing the same cache line as the previous one
	 *	ignore access, and don't train prefetcher
	 */
	if(line == last_accessed_line)
	{
		if(debug) fprintf(stderr, "Same line as last\n");
		return -1;
	}
		

	if(line == last_prefetched_line )
	{
		if(debug) fprintf(stderr, "[HIT] LPL:%d CRL:%d\n", last_prefetched_line, line);
		stat_total_prefetch_hit++;
		stat_heart_beat_prefetch_hit++;
	}

	/* Prefetch hit second calculation */
	it = prefetch_map.find(page);
	if(it != prefetch_map.end() && it->second->last_line != line)
	{
		if(it->second->prefetch_line == line) { stat_total_prefetch_hit2++; stat_heart_beat_prefetch_hit2++;}
		prefetch_map.erase(it);
	}
	
	last_prefetched_line = 0;
	last_accessed_line = line;

	int index = -1;
	for(int i=0;i<SIZE;++i)
	{
		if(trackers[i].valid && trackers[i].page == page)
		{
			index = i;
			break;
		}
	}

	/* If it misses the history cache, allocate one tracker */
	if(index == -1)
	{
		if(debug) fprintf(stderr, "HC Miss\n");
		int maxAge = -1, repIndex = -1;
		for(int i=0;i<SIZE;++i)
		{
			if(!trackers[i].valid)
			{
				repIndex = i;
				break;
			}
			else if(trackers[i].age > maxAge)
			{
				maxAge = trackers[i].age;
				repIndex = i;
			}
		}
		ASSERT(repIndex!=-1,"Cache lookup failed\n");
		index = repIndex;

		trackers[index].page = page;
		trackers[index].last_line = line_offset;
		trackers[index].stride = 0;
		trackers[index].age = 0;
		trackers[index].valid = true;
		for(int i=0;i<SIZE;++i){if(i!=index) trackers[i].age++;}
		if(debug) fprintf(stderr, "HC insert, P:%x LL:%d S:%d A:%d v:%d\n", trackers[index].page,
																			trackers[index].last_line,
																			trackers[index].stride,
																			trackers[index].age,
																			trackers[index].valid);
		return -1;
	}
	
	else
	{
		if(debug) fprintf(stderr, "HC Hit\n");
		if(debug) fprintf(stderr, "HC state, P:%x LL:%d S:%d A:%d v:%d\n", 	trackers[index].page,
																			trackers[index].last_line,
																			trackers[index].stride,
																			trackers[index].age,
																			trackers[index].valid);
		trackers[index].age = 0;
		trackers[index].valid = true;
		for(int i=0;i<SIZE;++i){if(i!=index) trackers[i].age++;}

		bool steady = false;
		int line_stride = line_offset - trackers[index].last_line;
		if(line_stride == 0)
		{
			if(debug) fprintf(stderr, "Line stride zero, returning\n");
			return -1;
		}
			
		if(debug) fprintf(stderr, "Stride nonzero\n");
		if(line_stride == trackers[index].stride && (line_offset+line_stride>=0 && line_offset+line_stride<64))
		{
			if(debug) fprintf(stderr, "Stride match:%d\n", line_stride);
			steady = true;
			(*prefAddr) = (page << PAGE_SIZE_LOG) + ((line_offset + line_stride) << LINE_SIZE_LOG);
			last_prefetched_line = (*prefAddr) >> LINE_SIZE_LOG;
			info_t* temp = (info_t*) malloc(sizeof(info_t));
			temp->last_line = line;
			temp->prefetch_line = last_prefetched_line;
			prefetch_map.insert(std::pair<unsigned int, info_t*>(page, temp));
			stat_total_prefetch++;
			stat_heart_beat_prefetch++;
		}

		trackers[index].page = page;
		trackers[index].last_line = line_offset;
		trackers[index].stride = line_stride;
		if(debug) fprintf(stderr, "HC update, P:%x LL:%d S:%d A:%d v:%d\n", trackers[index].page,
																			trackers[index].last_line,
																			trackers[index].stride,
																			trackers[index].age,
																			trackers[index].valid);
		
		return (steady?1:-1);
	}
}

void prefetcher_heartbeat_stats()
{
	fprintf(stdout, "Prefetch: %*d ", 6, stat_heart_beat_prefetch);
	fprintf(stdout, "Hit: %*d ", 6, stat_heart_beat_prefetch_hit);
	fprintf(stdout, "Hit2: %*d [%0.2f]\n", 6, stat_heart_beat_prefetch_hit2, (float)stat_heart_beat_prefetch_hit2/stat_heart_beat_prefetch*100);
	stat_heart_beat_prefetch = 0;
	stat_heart_beat_prefetch_hit = 0;
	stat_heart_beat_prefetch_hit2 = 0;
}

void prefetcher_final_stats()
{
	fprintf(stdout, "Trackers: %d\n", SIZE);
	fprintf(stdout, "Total memory accesses: %d\n", stat_total_mem_access);
	fprintf(stdout, "Total prefetch: %d \n", stat_total_prefetch);
	fprintf(stdout, "Total prefetch hit: %d [%0.2f]\n", stat_total_prefetch_hit, (float)stat_total_prefetch_hit/stat_total_prefetch*100);
	fprintf(stdout, "Total prefetch hit2: %d [%0.2f]\n", stat_total_prefetch_hit2, (float)stat_total_prefetch_hit2/stat_total_prefetch*100);
}

void prefetcher_destroy()
{
	if(debug)fprintf(stderr, "Deallocating stream prefetcher\n");
}
