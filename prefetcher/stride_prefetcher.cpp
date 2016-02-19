#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "prefetcher.h"


/* 	This file implements stride directed prefetching, 
 *	a MICRO'92 paper by Wu, Patel et. al
 */
	
#define SIZE 16

typedef struct tracker
{
	unsigned int _pc;
	unsigned int _lastAddr;
	int _stride;
	bool _valid;
	unsigned int _age;
}tracker;

tracker *trackers;
unsigned int last_prefetch_address;

unsigned int stat_totalMemAccess = 0;
unsigned int stat_total_prefetch = 0;
unsigned int stat_total_prefetch_hit = 0;

void prefetcher_init()
{
	fprintf(stderr, "Stride Prefetcher: Tracker size=%d\n", SIZE);
	trackers = (tracker*)malloc(SIZE*sizeof(tracker));
	ASSERT(trackers!=NULL,"Trackers malloc failed!\n");
	for(int i=0;i<SIZE;++i)
		trackers[i]._valid = false;
	last_prefetch_address = 0;
}

int prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{
	int index, repIndex, currStride; 
	unsigned int maxAge;

	stat_totalMemAccess++;
	if(last_prefetch_address!=0 && addr == last_prefetch_address)
		stat_total_prefetch_hit++;

	index = -1;
	for(int i=0; i<SIZE; ++i)
	{
		if(pc == trackers[i]._pc && trackers[i]._valid)
		{
			index = i;
			break;
		}
	}
	
	if(index == -1)
	{
		maxAge = 0;
		repIndex = 0;
		for(int i=0; i<SIZE; ++i)
		{
			if(!trackers[i]._valid)
			{
				repIndex = i;
				break;
			}
			else if(trackers[i]._age > maxAge)
			{
				maxAge = trackers[i]._age;
				repIndex = i;
			}
		}
		
		trackers[repIndex]._pc = pc;
		trackers[repIndex]._lastAddr = addr;
		trackers[repIndex]._stride = 0;
		trackers[repIndex]._valid = true;
		trackers[repIndex]._age = 0;
		last_prefetch_address = 0;
		for(int i=0;i<SIZE;++i) {if(i!=repIndex)trackers[i]._age++;}
		return -1;
	}
	else
	{
		bool steady = false;
		currStride = addr - trackers[index]._lastAddr;
		if(currStride == trackers[index]._stride)
		{
			(*prefAddr) = addr + trackers[index]._stride;
			last_prefetch_address = (*prefAddr);
			stat_total_prefetch++;
			steady = true;
		}
		else
			last_prefetch_address = 0;
		trackers[index]._lastAddr = addr;
		trackers[index]._stride = currStride;
		trackers[index]._age = 0;
		for(int i=0;i<SIZE;++i) {if(i!=index)trackers[i]._age++;}
		
		return (steady?1:-1);
	}
}

void prefetcher_heartbeat_stats()
{
	
}

void prefetcher_final_stats()
{
	fprintf(stdout, "\nTotal Memory Access: %d\n", stat_totalMemAccess);
	fprintf(stdout, "Total prefetcher prediction: %d [%0.2f]\n", stat_total_prefetch, (float)stat_total_prefetch/stat_totalMemAccess*100);
	fprintf(stdout, "Total prediction hit: %d [%0.2f]\n", stat_total_prefetch_hit, (float)stat_total_prefetch_hit/stat_total_prefetch*100);
}

void prefetcher_destroy()
{
	fprintf(stderr, "Deallocating stride prefetcher\n");
	free(trackers);
}