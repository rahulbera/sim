#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TwoLevelPrefetcher.h"

#define LINE_SIZE_LOG 6
#define REGION_SIZE_LOG 12
#define OFFSET_LOG (REGION_SIZE_LOG - LINE_SIZE_LOG)
#define MAX_OFFSET (1 << (OFFSET_LOG))

void TwoLevelPrefetcher::prefetcher_init(char *s, bool d, int n)
{

}

void TwoLevelPrefetcher::prefetcher_init(char *s, bool d, unsigned int map_size, unsigned int trackers_per_reg)
{
	strcpy(name, s);
	debug = d;
	region_map_size = map_size;
	trackers_per_region = trackers_per_reg;
	trackers_size = trackers_per_region*region_map_size;

	region_map = (entry_t*)malloc(region_map_size*sizeof(entry_t));
	ASSERT(region_map!=NULL, "Region map allocation failed!\n");
	for(int i=0;i<region_map_size;++i)
	{
		region_map[i].age = 0;
		region_map[i].valid = false;
	}

	trackers = (tracker_t*)malloc(trackers_size*sizeof(tracker_t));
	ASSERT(trackers!=NULL, "Trackers allocation failed!\n");
	for(int i=0;i<trackers_size;++i)
	{
		trackers[i].age = 0;
		trackers[i].valid = false;
	}

	stat_total_prefetch = stat_total_region_map_repl = stat_total_trackers_repl = 0;
}

int TwoLevelPrefetcher::prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{
	unsigned int region = addr >> REGION_SIZE_LOG;
	unsigned int offset = (addr >> LINE_SIZE_LOG) & (MAX_OFFSET - 1);
	int index = -1;

	/* Check region_map table for current region */
	for(int i=0;i<region_map_size;++i)
	{
		if(region_map[i].valid && region_map[i].region == region)
		{
			index = i;
			break;
		}
	}

	/* If it hits in region_map */
	if(index != -1)
	{
		int start = (index * trackers_per_region);
		int end = (index+1) * trackers_per_region - 1;
		int index2 = -1;

		/* Check for corresponding PC */
		for(int i=start;i<=end;++i)
		{
			if(trackers[i].valid && trackers[i].pc == pc)
			{
				index2 = i;
				break;
			}
		}

		/* if it hits in tracker table */
		if(index2 != -1)
		{
			int prevStride = trackers[index2].stride;
			int currStride = offset - trackers[index2].last_offset;
			if(currStride == 0)
				return -1;
			trackers[index2].last_offset = offset;
			trackers[index2].stride = currStride;
			for(int i=start;i<=end;++i) trackers[i].age++;
			trackers[index2].age = 0;

			if(currStride != prevStride)
				return -1;

			(*prefAddr) = (region << REGION_SIZE_LOG) + ((offset + currStride) << LINE_SIZE_LOG);
			stat_total_prefetch++;
			return 1;
		}
		else
		{
			int repIndex = -1, maxAge = -1;
			for(int i=start;i<=end;++i)
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
			ASSERT(repIndex!=-1, "Invalid trackers replacement index\n");
			stat_total_trackers_repl++;
			trackers[repIndex].pc = pc;
			trackers[repIndex].last_offset = offset;
			trackers[repIndex].stride = 0;
			trackers[repIndex].valid = true;
			for(int i=start;i<=end;++i) trackers[i].age++;
			trackers[repIndex].age = 0;

			return -1;
		}
	}

	/* If code it still here, that means it missed region map */
	int repIndex = -1, maxAge = -1;
	for(int i=0;i<region_map_size;++i)
	{
		if(!region_map[i].valid)
		{
			repIndex = i;
			break;
		}
		else if(region_map[i].age > maxAge)
		{
			maxAge = region_map[i].age;
			repIndex = i;
		}
	}
	ASSERT(repIndex!=-1, "Invalid region_map replacement index\n");
	stat_total_region_map_repl++;
	int start = repIndex * trackers_per_region;
	int end = (repIndex+1) * trackers_per_region - 1;
	for(int i=start;i<=end;++i)
	{
		trackers[i].age = 0;
		trackers[i].valid = false;
	}
	region_map[repIndex].region = region;
	region_map[repIndex].valid = true;
	for(int i=0;i<region_map_size;++i) region_map[i].age++;
	region_map[repIndex].age = 0;

	return -1;
}

void TwoLevelPrefetcher::prefetcher_heartbeat_stats()
{

}

void TwoLevelPrefetcher::prefetcher_final_stats()
{
	fprintf(stdout, "[ Prefetcher.%s ]\n", name);
	fprintf(stdout, "Type = TwoLevel\n");
	fprintf(stdout, "Reiogn = %dB\n", 1<<REGION_SIZE_LOG);
	fprintf(stdout, "RegionMap = %d\n", region_map_size);
	fprintf(stdout, "TrackersPerRegion = %d\n", trackers_per_region);
	fprintf(stdout, "TotalPrefetch = %d \n", stat_total_prefetch);
	fprintf(stdout, "RegionMapRepl = %d\n", stat_total_region_map_repl);
	fprintf(stdout, "TrackersRepl = %d\n", stat_total_trackers_repl);
}

void TwoLevelPrefetcher::prefetcher_destroy()
{
	free(region_map);
	free(trackers);
}