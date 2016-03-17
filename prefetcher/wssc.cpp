#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wssc.h"

#define LINE_SIZE_LOG 6
#define REGION_SIZE_LOG 12
#define OFFSET_LOG (REGION_SIZE_LOG - LINE_SIZE_LOG)
#define MAX_OFFSET (1<<(OFFSET_LOG))

wssc::wssc(char *s, uint n, uint a, ulong i)
{
	strcpy(name,s);
	size = n;
	associativity = a;
	sets = size/associativity;
	setsInLog = log2(sets);
	interval = i;

	map = (wssc_t**)malloc(sets * sizeof(wssc_t*));
	ASSERT(map!=NULL, "WSSC Allocation1 failed!\n");
	for(uint i=0;i<sets;++i)
	{
		map[i] = (wssc_t*)malloc(associativity * sizeof(wssc_t));
		ASSERT(map[i]!=NULL, "WSSC Allocation2 failed!\n");
	}
	for(uint i=0;i<sets;++i)
	{
		for(uint j=0;j<associativity;++j)
		{
			map[i][j].pattern = (bool*)malloc(MAX_OFFSET * sizeof(bool));
			ASSERT(map[i][j].pattern!=NULL, "WSSC pattern allocation failed!\n");
			for(uint k=0;k<MAX_OFFSET;++k) map[i][j].pattern[k] = false;
			map[i][j].valid = false;
		}
	}

	total_invalidation = 0;
}

wssc::~wssc()
{
	for(int i=0;i<sets;++i)
	{
		for(uint j=0;j<associativity;++j)
			free(map[i][j].pattern);
	}
	free(map);
}

uint wssc::log2(uint n)
{
	if(n==1) return 0;
	else return (1+log2(n/2));
}

ulong wssc::get_interval()
{
	return interval;
}

void wssc::link_prefetcher(SMSPrefetcher *pref)
{
	prefetcher = pref;
}

bool wssc::find(uint page, uint *s, uint *w, uint *t)
{
	uint setIndex = page & (sets-1);
	uint tag = page >> setsInLog;
	bool found = false;
	uint wayIndex;
	for(uint i=0;i<associativity;++i)
	{
		if(map[setIndex][i].valid && map[setIndex][i].tag == tag)
		{
			found = true;
			wayIndex = i;
			break;
		}
	}
	(*s) = setIndex;
	(*w) = wayIndex;
	(*t) = tag;
	return found;
}

/* L2 cache will call this function
 * when interval gets over.
 */
void wssc::invalidate()
{
	total_invalidation++;
	for(uint i=0;i<sets;++i)
	{
		for(uint j=0;j<associativity;++j)
		{
			for(uint k=0;k<MAX_OFFSET;++k) map[i][j].pattern[k] = false;
			map[i][j].valid = false;
		}		
	}
	prefetcher->update_tc_uc();
	#ifdef DEBUG
		fprintf(stderr, "WSSC invalidated\n");
	#endif
}

/* Prefetcher calls this function
 * during issuing a new prefetch vector
 */
void wssc::insert(uint page, ulong pht_tag, bool *pattern, uint n)
{
	uint setIndex, wayIndex, tag;
	bool found = find(page, &setIndex, &wayIndex, &tag);

	#ifdef DEBUG
		fprintf(stderr, "WSSC insert:P:%x,PHT_T:%lx,N:%d,F:%d,S:%d,W:%d,T:%x\n", 	page, pht_tag, n,
																				found, setIndex, wayIndex, tag);
	#endif

	/* If the page is already being monitored by wssc */
	if(found)
	{
		#ifdef DEBUG
			fprintf(stderr, "New page access: found in WSSC, S:%d W:%d\n", setIndex, wayIndex);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		if(map[setIndex][wayIndex].pht_tag == pht_tag)
		{
			#ifdef DEBUG
				fprintf(stderr, "Accessed by same pht_tag:%lx\n", pht_tag);
				fprintf(stderr, "Incrementing TC of %lx by %d\n", pht_tag, n);
			#endif
			prefetcher->incr_tc(pht_tag, n);
			for(uint i=0;i<MAX_OFFSET;++i)
				map[setIndex][wayIndex].pattern[i] = (map[setIndex][wayIndex].pattern[i] | pattern[i]);
		}
		else
		{
			#ifdef DEBUG
				fprintf(stderr, "Different pht_tag accessing same page %lx and %lx\n", map[setIndex][wayIndex].pht_tag, pht_tag);
			#endif
			map[setIndex][wayIndex].pht_tag = pht_tag;
			prefetcher->incr_tc(pht_tag, n);
			for(uint i=0;i<MAX_OFFSET;++i)
				map[setIndex][wayIndex].pattern[i] = pattern[i];
		}
	}

	/* If the page is not found inside wssc */
	else
	{
		#ifdef DEBUG
			fprintf(stderr, "Page not found in WSSC\n");
		#endif
		int repIndex = -1;
		for(uint i=0;i<associativity;++i)
		{
			if(!map[setIndex][i].valid)
			{
				repIndex = i;
				break;
			}
		}
		if(repIndex != -1)
		{
			#ifdef DEBUG
				fprintf(stderr, "Rep candidate found:[%*d,%d]\n", 3, setIndex, repIndex);
			#endif
			prefetcher->incr_tc(pht_tag, n);
			map[setIndex][repIndex].tag = tag;
			map[setIndex][repIndex].pht_tag = pht_tag;
			for(uint i=0;i<64;++i) map[setIndex][repIndex].pattern[i] = pattern[i];
			map[setIndex][repIndex].valid = true;
		}
	}
}


/* Prefetcher calls this function
 * seeing a demand access to page
 */
void wssc::update(uint page, uint offset)
{
	uint setIndex, wayIndex, tag;
	bool found = find(page, &setIndex, &wayIndex, &tag);

	if(found)
	{
		#ifdef DEBUG
			fprintf(stderr, "Demand access to page:%x offset:%d found in WSSC\n", page, offset);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		if(map[setIndex][wayIndex].pattern[offset])
		{
			prefetcher->incr_uc(map[setIndex][wayIndex].pht_tag);
			map[setIndex][wayIndex].pattern[offset] = false;
		}
	}
}

void wssc::heart_beat_stats()
{

}

void wssc::final_stats()
{
	fprintf(stdout, "[ WSSC.%s ]\n", name);
	fprintf(stdout, "Size = %d\n", size);
	fprintf(stdout, "Associativity = %d\n", associativity);
	fprintf(stdout, "Interval = %lu\n", interval);
	fprintf(stdout, "Invalidation = %d\n", total_invalidation);
}