#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wssc.h"
#include "../common/vars.h"

uint wssc::log2(uint n)
{
	if(n==1) return 0;
	else return (1+log2(n/2));
}

wssc::wssc()
{

}

wssc::~wssc()
{
	free(plt);
	free(sat);
}

void wssc::init(char *s, uint plt_s, uint plt_a, uint sat_a, ulong i, float t)
{
	strcpy(name,s);
	interval = i;
	threshold = t;

	plt_size = plt_s;
	plt_assoc = plt_a;
	plt_sets = plt_size/plt_assoc;
	plt_sets_in_log = log2(plt_sets);
	sat_size = plt_s * sat_a;
	sat_assoc = sat_a;
	sat_sets = plt_s;
	sat_sets_in_log = log2(sat_sets);

	plt = (plt_t**)malloc(plt_sets*sizeof(plt_t*));
	ASSERT(plt!=NULL,"PLT malloc failed 1\n");
	for(uint i=0;i<plt_sets;++i)
	{
		plt[i] = (plt_t*)malloc(plt_assoc*sizeof(plt_t));
		ASSERT(plt[i]!=NULL,"PLT malloc failed 2\n");
	}
	for(uint i=0;i<plt_sets;++i)
	{
		for(uint j=0;j<plt_assoc;++j)
			plt[i][j].valid = false;
	}

	sat = (sat_t**)malloc(sat_sets*sizeof(sat_t*));
	ASSERT(sat!=NULL,"SAT malloc failed 1\n");
	for(uint i=0;i<sat_sets;++i)
	{
		sat[i] = (sat_t*)malloc(sat_assoc*sizeof(sat_t));
		ASSERT(sat[i]!=NULL, "SAT malloc falied 2\n");
	}
	for(uint i=0;i<sat_sets;++i)
	{
		for(uint j=0;j<sat_assoc;++j)
		{
			sat[i][j].pattern = (bool*)malloc(MAX_OFFSET*sizeof(bool));
			ASSERT(sat[i][j].pattern!=NULL, "SAT pattern malloc failed!\n");
			for(uint k=0;k<MAX_OFFSET;++k) sat[i][j].pattern[k] = false;
			sat[i][j].age = 0;
			sat[i][j].valid = false;
		}
	}

	total_invalidation = 0;
	total_access = 0;
	total_insert = 0;
	replacementNeeded = 0;
	samePHTAccess = 0;
	diffPHTAccess = 0;
}


ulong wssc::get_interval()
{
	return interval;
}

float wssc::get_threshold()
{
	return threshold;
}

void wssc::link_prefetcher(SMSPrefetcher *pref)
{
	prefetcher = pref;
}

bool wssc::plt_find(uint page, uint *s, uint *w, uint *t)
{
	uint setIndex = page & (plt_sets-1);
	uint tag = page >> plt_sets_in_log;
	bool found = false;
	uint wayIndex;
	for(uint i=0;i<plt_assoc;++i)
	{
		if(plt[setIndex][i].valid && plt[setIndex][i].tag == tag)
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
	//dump(total_invalidation);
	//prefetcher->dump(total_invalidation);
	for(uint i=0;i<plt_sets;++i)
	{
		for(uint j=0;j<plt_assoc;++j)
			plt[i][j].valid = false;
	}
	for(uint i=0;i<sat_sets;++i)
	{
		for(uint j=0;j<sat_assoc;++j)
			sat[i][j].valid = false;
	}
	prefetcher->update_tc_uc();
	#ifdef DEBUG
		fprintf(stderr, "WSSC invalidated\n");
	#endif
}

/* Prefetcher calls this function
 * during issuing a new prefetch vector
 * Returns true if it actually creates 
 * a new tracker. False otherwise.
 */
bool wssc::insert(uint page, ulong pht_tag, bool *pattern, uint n)
{
	total_access++;
	uint setIndex, wayIndex, tag;
	bool found = plt_find(page, &setIndex, &wayIndex, &tag);

	#ifdef DEBUG
		fprintf(stderr, "WSSC insert:P:%x,PHT_T:%lx,N:%d,F:%d,S:%d,W:%d,T:%x\n", 	page, pht_tag, n,
																					found, setIndex, wayIndex, tag);
	#endif

	/* If the page is found in PLT */
	if(found)
	{
		#ifdef DEBUG
			fprintf(stderr, "Page %x found in PLT [%d,%d]\n", page, setIndex, wayIndex);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		uint sat_set_index = (setIndex * plt_assoc) + wayIndex;
		ASSERT((sat_set_index>=0 && sat_set_index<sat_sets), "Invalid SAT set index\n");

		/* Find the PHT in SAT */
		int sat_way_index = -1;
		for(int i=0;i<sat_assoc;++i)
		{
			if(sat[sat_set_index][i].valid && sat[sat_set_index][i].pht_tag == pht_tag)
			{
				sat_way_index = i;
				break;
			}
		}
		/* If found in SAT */
		if(sat_way_index != -1)
		{
			samePHTAccess++;
			#ifdef DEBUG
				fprintf(stderr, "PHT tag %lx found\n", pht_tag);
				debug_sat_entry(sat_set_index, sat_way_index);
			#endif
			uint m = 0;
			for(uint i=0;i<MAX_OFFSET;++i)
			{ 
				if(sat[sat_set_index][sat_way_index].pattern[i] && pattern[i]) m++;
				sat[sat_set_index][sat_way_index].pattern[i] = sat[sat_set_index][sat_way_index].pattern[i] | pattern[i];
			}
			for(uint i=0;i<sat_assoc;++i) sat[sat_set_index][i].age++;
			sat[sat_set_index][sat_way_index].age = 0;
			prefetcher->incr_tc(pht_tag, n-m);
			#ifdef DEBUG
				debug_sat_entry(sat_set_index,sat_way_index);
			#endif
			return false;
		}
		/* If it's not in SAT */ 
		else
		{
			diffPHTAccess++;
			#ifdef DEBUG
				fprintf(stderr, "PHT tag %lx not found\n", pht_tag);
			#endif
			int repIndex = -1;
			int maxAge = -1;
			for(int i=0;i<sat_assoc;++i)
			{
				if(!sat[sat_set_index][i].valid)
				{
					repIndex = i;
					break;
				}
				else if(sat[sat_set_index][i].age > maxAge)
				{
					repIndex = i;
					maxAge = sat[sat_set_index][i].age;
				}
			}
			ASSERT(repIndex!=-1, "Invalid SAT replacement index\n");
			#ifdef DEBUG
				fprintf(stderr, "SAT replacement index [%d,%d]\n", sat_set_index, repIndex);
				debug_sat_entry(sat_set_index, repIndex);
			#endif
			sat[sat_set_index][repIndex].pht_tag = pht_tag;
			for(uint i=0;i<MAX_OFFSET;++i) sat[sat_set_index][repIndex].pattern[i] = pattern[i];
			sat[sat_set_index][repIndex].valid = true;
			for(uint i=0;i<sat_assoc;++i) sat[sat_set_index][i].age++;
			sat[sat_set_index][repIndex].age = 0;
			prefetcher->incr_tc(pht_tag, n);
			#ifdef DEBUG
				debug_sat_entry(sat_set_index, repIndex);
			#endif
			return true;
		}
	}
	/* if the page is not at all tracked by PLT */
	else
	{
		#ifdef DEBUG
			fprintf(stderr, "Page %x not found in PLT\n", page);
		#endif
		int repIndex = -1;
		for(int i=0;i<plt_assoc;++i)
		{
			if(!plt[setIndex][i].valid)
			{
				repIndex = i;
				break;
			}
		}
		if(repIndex == -1)
		{
			replacementNeeded++;
			#ifdef DEBUG
				fprintf(stderr, "No replacement candidate found!\n");
			#endif
			return false;
		}
		else
		{
			total_insert++;
			#ifdef DEBUG
				fprintf(stderr, "Rep candidate found:[%*d,%*d]\n", 3, setIndex, 2, repIndex);
			#endif
			plt[setIndex][repIndex].tag = tag;
			plt[setIndex][repIndex].valid = true;
			uint sat_set_index = (setIndex * plt_assoc) + repIndex;
			ASSERT(sat[sat_set_index][0].valid==false, "Invalid SAT insertion for first time\n");
			sat[sat_set_index][0].pht_tag = pht_tag;
			for(uint i=0;i<MAX_OFFSET;++i) sat[sat_set_index][0].pattern[i] = pattern[i];
			sat[sat_set_index][0].age = 0;
			sat[sat_set_index][0].valid = true;
			prefetcher->incr_tc(pht_tag, n);
		}
	}
}


/* Prefetcher calls this function
 * seeing a demand access to page
 */
void wssc::update(uint page, uint offset)
{
	uint setIndex, wayIndex, tag;
	bool found = plt_find(page, &setIndex, &wayIndex, &tag);

	if(found)
	{
		#ifdef DEBUG
			fprintf(stderr, "Demand access to page:%x offset:%d found in WSSC\n", page, offset);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		uint sat_set_index = (setIndex * plt_assoc) + wayIndex;
		ASSERT((sat_set_index>=0 && sat_set_index<sat_sets), "Invalid SAT set index\n");
		for(uint i=0;i<sat_assoc;++i)
		{
			if(sat[sat_set_index][i].valid && sat[sat_set_index][i].pattern[offset])
			{
				sat[sat_set_index][i].pattern[offset] = false;
				prefetcher->incr_uc(sat[sat_set_index][i].pht_tag);
			}
		}
	}
}

void wssc::dump(uint n)
{
	/*char filename[20];
	sprintf(filename, "debug/WSSC.%d",n);
	FILE *fp = fopen(filename,"w");
	ASSERT(fp!=NULL, "WSSC dump file creation failed!\n");
	for(int i=0;i<sets;++i)
	{
		fprintf(fp, "Set:%d\n", i);
		for(int j=0;j<associativity;++j)
		{
			fprintf(fp, "[%d]%*x|%*lx|", j, 3, map[i][j].tag, 10, map[i][j].pht_tag);
			fprintf(fp, "%*d|%*x|", 4, (uint)(map[i][j].pht_tag & 2047), 7, (uint)(map[i][j].pht_tag >> 11));
			for(int k=0;k<MAX_OFFSET;++k)
				fprintf(fp, "%d", map[i][j].pattern[k]);
			fprintf(fp, "|%d\n", map[i][j].valid);
		}
	}
	fflush(fp);
	fclose(fp);*/
}

void wssc::heart_beat_stats()
{

}

void wssc::final_stats()
{
	uint size = (plt_size*(20-plt_sets_in_log+1)+sat_size*(38+64+1))/(8*1024);
	fprintf(stdout, "[ WSSC.%s ]\n", name);
	fprintf(stdout, "PLTSize = %d\n", plt_size);
	fprintf(stdout, "PLTAssoc = %d\n", plt_assoc);
	fprintf(stdout, "SATAssoc = %d\n", sat_assoc);
	fprintf(stdout, "Threshold = %0.3f\n", threshold);
	fprintf(stdout, "Interval = %ld\n", interval);
	fprintf(stdout, "Invalidaion = %d\n", total_invalidation);
	fprintf(stdout, "Size = %dKB\n", size);
	fprintf(stdout, "TotalAccess = %d\n", total_access);
	fprintf(stdout, "ReplcementNeeded = %d\n", replacementNeeded);
	fprintf(stdout, "ReplcementFound = %d\n", total_insert);
	fprintf(stdout, "SamePHTAccess = %d\n", samePHTAccess);
	fprintf(stdout, "DiffPHTAccess = %d\n", diffPHTAccess);
}

void wssc::debug_sat_entry(uint setIndex, uint wayIndex)
{
	fprintf(stderr, "SAT:<%lx|", sat[setIndex][wayIndex].pht_tag);
	for(uint i=0;i<MAX_OFFSET;++i) fprintf(stderr, "%d", sat[setIndex][wayIndex].pattern[i]);
	fprintf(stderr, "|%d|%d>\n", sat[setIndex][wayIndex].age, sat[setIndex][wayIndex].valid);
}