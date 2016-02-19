#include <stdio.h>
#include <stdlib.h>
#include "prefetcher.h"

#define size 16
#define interval 256
#define THRESHOLD 4

typedef struct entry
{
	int stride;
	int confidence;
	int age;
}entry_t;

entry_t stride_cache[size];
int current_stride;
bool prefetcherOff;
unsigned int last_address;
unsigned int prevPrefetchAddr;


/* Stats counters */
unsigned int stat_totalMemAccess = 0;
unsigned int stat_totalPrediction = 0;
unsigned int stat_totalHitPrediction = 0;

bool get_index(int value, int *index)
{
	bool hit = false;
	int minConf = 0x7fffffff, minConfIndex = -1;
	int i;
	for(i=0;i<size;++i)
	{
		if(stride_cache[i].stride == value)
		{
			hit = true;
			break;
		}
		if(stride_cache[i].confidence < minConf)
		{
			minConf = stride_cache[i].confidence;
			minConfIndex = i;
		}
	}
	if(hit)
	{
		(*index) = i;
		return true;
	}
	else
	{
		(*index) = minConfIndex;
		return false;
	}
}


void prefetcher_init()
{
	fprintf(stderr,"Stride cache prefetcher.\n");
	for(int i=0;i<size;++i)
	{
		stride_cache[i].stride = stride_cache[i].confidence = 0;
		stride_cache[i].age = -1;
	}
	current_stride = 1;
	last_address = 0;
	prefetcherOff = false;
}

int prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *perfAddr)
{
	stat_totalMemAccess++;
	if(addr == prevPrefetchAddr)
		stat_totalHitPrediction++;

	int stride = addr - last_address;
	int sc_index = -1;
	bool hit = get_index(stride, &sc_index);
	ASSERT(sc_index!=-1,"Stride Cache lookup failed\n");
	if(hit)
		stride_cache[sc_index].confidence++;
	else
	{
		stride_cache[sc_index].stride = stride;
		stride_cache[sc_index].confidence = 1;
	}

	if(stat_totalMemAccess%256==0)
	{
		int maxConf = -1, maxConfIndex = -1;
		int i;
		for(i=0;i<size;++i)
		{
			if(stride_cache[i].confidence > maxConf)
			{
				maxConf = stride_cache[i].confidence;
				maxConfIndex = i;
			}
		}
		if(maxConf >= THRESHOLD)
			current_stride = stride_cache[maxConfIndex].stride;
		else
			prefetcherOff = true;
	}

	if(!prefetcherOff)
	{
		stat_totalPrediction++;
		(*perfAddr) = addr + current_stride;
		prevPrefetchAddr = (*perfAddr);
		return 1;
	}
	else
		return -1;
		
}

void prefetcher_heartbeat_stats()
{
	
}

void prefetcher_final_stats()
{
	fprintf(stdout, "\nTotal Memory Access: %d\n", stat_totalMemAccess);
	fprintf(stdout, "Total prefetcher prediction: %d [%0.2f]\n", stat_totalPrediction, (float)stat_totalPrediction/stat_totalMemAccess*100);
	fprintf(stdout, "Total prediction hit: %d [%0.2f]\n", stat_totalHitPrediction, (float)stat_totalHitPrediction/stat_totalPrediction*100);
}

void prefetcher_destroy()
{
	fprintf(stderr, "Deallocating stride prefetcher\n");
}