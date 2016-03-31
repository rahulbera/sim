#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wssc.h"
#include "../common/vars.h"

wssc::wssc()
{

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

void wssc::init(char *s, uint n, uint a, ulong i)
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
	total_wssc_access = 0;
	total_insert = 0;
	samePHTAccess = 0;
	replacementNeeded = 0;
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
	//dump(total_invalidation);
	//prefetcher->dump(total_invalidation);
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
 * Returns true if it actually creates 
 * a new page tracker. False if WSSC is
 * already monitoring the page from past.
 */
bool wssc::insert(uint page, ulong pht_tag, bool *pattern, uint n)
{
	total_wssc_access++;
	uint setIndex, wayIndex, tag;
	bool found = find(page, &setIndex, &wayIndex, &tag);

	#ifdef DEBUG
		fprintf(stderr, "WSSC insert:P:%x,PHT_T:%lx,N:%d,F:%d,S:%d,W:%d,T:%x\n", 	page, pht_tag, n,
																					found, setIndex, wayIndex, tag);
	#endif

	/* If the page is already being monitored by wssc */
	if(found)
	{
		total_insert++;
		uint pc = (uint)(pht_tag >> 6);
		std::unordered_map<uint,counter_t*>::iterator it = pc_prefetch_map.find(pc);
		if(it != pc_prefetch_map.end())
			it->second->counter1 += n;
		else
		{
			counter_t *temp = (counter_t*)malloc(sizeof(counter_t));
			temp->counter1 = n;
			temp->counter2 = 0;
			pc_prefetch_map.insert(std::pair<uint,counter_t*>(pc,temp));
		}
		#ifdef DEBUG
			fprintf(stderr, "New page access: found in WSSC, S:%d W:%d\n", setIndex, wayIndex);
			debug_wssc_entry(setIndex, wayIndex);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		if(map[setIndex][wayIndex].pht_tag == pht_tag)
		{
			samePHTAccess++;
			#ifdef DEBUG
				fprintf(stderr, "Accessed by same pht_tag:%lx\n", pht_tag);
				fprintf(stderr, "Incrementing TC of %lx by %d\n", pht_tag, n);
			#endif
			uint m = 0;
			for(uint i=0;i<MAX_OFFSET;++i) {if(map[setIndex][wayIndex].pattern[i] && pattern[i]) m++;}
			for(uint i=0;i<MAX_OFFSET;++i)
				map[setIndex][wayIndex].pattern[i] = (map[setIndex][wayIndex].pattern[i] | pattern[i]);
			prefetcher->incr_tc(pht_tag, (n-m));
		}
		else
		{
			#ifdef DEBUG
				fprintf(stderr, "Different pht_tag accessing same page %lx and %lx\n", map[setIndex][wayIndex].pht_tag, pht_tag);
				fprintf(stderr, "PHT of PC:%x access collision to same page %x by PC:%x", (uint)(map[setIndex][wayIndex].pht_tag>>6), page, (uint)(pht_tag>>6));
			#endif
			map[setIndex][wayIndex].pht_tag = pht_tag;
			for(uint i=0;i<MAX_OFFSET;++i)
				map[setIndex][wayIndex].pattern[i] = pattern[i];
			prefetcher->incr_tc(pht_tag, n);
		}
		#ifdef DEBUG
			debug_wssc_entry(setIndex,wayIndex);
		#endif
		return false;
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
			total_insert++;
			uint pc = (uint)(pht_tag >> 6);
			std::unordered_map<uint,counter_t*>::iterator it = pc_prefetch_map.find(pc);
			if(it != pc_prefetch_map.end())
				it->second->counter1 += n;
			else
			{
				counter_t *temp = (counter_t*)malloc(sizeof(counter_t));
				temp->counter1 = n;
				temp->counter2 = 0;
				pc_prefetch_map.insert(std::pair<uint,counter_t*>(pc,temp));
			}
			#ifdef DEBUG
				fprintf(stderr, "Rep candidate found:[%*d,%d]\n", 3, setIndex, repIndex);
				debug_wssc_entry(setIndex,repIndex);
			#endif
			prefetcher->incr_tc(pht_tag, n);
			map[setIndex][repIndex].tag = tag;
			map[setIndex][repIndex].pht_tag = pht_tag;
			for(uint i=0;i<64;++i) map[setIndex][repIndex].pattern[i] = pattern[i];
			map[setIndex][repIndex].valid = true;
			#ifdef DEBUG
				debug_wssc_entry(setIndex, repIndex);
			#endif
			return true;
		}
		else
		{
			replacementNeeded++;
			return false;
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
			debug_wssc_entry(setIndex,wayIndex);
		#endif
		ASSERT(setIndex>=0 && wayIndex>=0, "Invalid setIndex/wayIndex\n");
		if(map[setIndex][wayIndex].pattern[offset])
		{
			#ifdef DEBUG
				fprintf(stderr, "WSSC hit at offset %d\n", offset);
			#endif
			prefetcher->incr_uc(map[setIndex][wayIndex].pht_tag);
			map[setIndex][wayIndex].pattern[offset] = false;
			#ifdef DEBUG
				debug_wssc_entry(setIndex,wayIndex);
			#endif
			uint pc = (uint)(map[setIndex][wayIndex].pht_tag >> 6);
			std::unordered_map<uint,counter_t*>::iterator it = pc_prefetch_map.find(pc);
			if(it != pc_prefetch_map.end())
				it->second->counter2++;
		}
	}
}

void wssc::dump(uint n)
{
	char filename[20];
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
	fclose(fp);
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
	fprintf(stdout, "TotalAccess = %d\n", total_wssc_access);
	fprintf(stdout, "TotalInsertion = %d\n", total_insert);
	fprintf(stdout, "ReplcementNeeded = %d\n", replacementNeeded);
	fprintf(stdout, "SamePHTAccess = %d\n", samePHTAccess);
	fprintf(stdout, "WSSC.PC_PREFECTH_MAP\n");
	std::unordered_map<uint,counter_t*>::iterator it;
	for(it=pc_prefetch_map.begin();it!=pc_prefetch_map.end();++it)
		fprintf(stdout, "%*x %*d %*d\n", 8, it->first, 8, it->second->counter1, 8, it->second->counter2);
}

void wssc::debug_wssc_entry(uint setIndex, uint wayIndex)
{
	fprintf(stderr, "WSSC:<%*x|%*lx|", 6, map[setIndex][wayIndex].tag, 10, map[setIndex][wayIndex].pht_tag);
	for(int i=0;i<MAX_OFFSET;++i) fprintf(stderr, "%d", map[setIndex][wayIndex].pattern[i]);
	fprintf(stderr, "|%d>\n", map[setIndex][wayIndex].valid);
}